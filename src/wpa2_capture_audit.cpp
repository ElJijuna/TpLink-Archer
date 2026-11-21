#include "archer/dhcp_selecting.hpp"
#include "archer/credential_pipe.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
namespace fs=std::filesystem;
std::vector<std::uint8_t> read_capture(const fs::path& path){
 if(!fs::is_regular_file(path)||fs::file_size(path)>20480)throw std::runtime_error("Invalid capture file");
 std::ifstream f(path,std::ios::binary);std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(f)),{});if(f.bad()||b.empty())throw std::runtime_error("Capture read failed");return b;
}
int main(int argc,char** argv){
 if(argc!=2&&argc!=3){std::cerr<<"Usage: wpa2-capture-audit REFERENCE_CAPTURE [COMPARE_ATTEMPT_CAPTURE] (private credential FD required)\n";return 2;}
 try{
  const auto value=std::getenv("ARCHER_CREDENTIAL_FD");char* end=nullptr;errno=0;long fd=value?std::strtol(value,&end,10):-1;
  if(!value||errno||!end||*end||fd<3||fd>0x7fffffff)throw std::runtime_error("Private credential FD required");
  auto pmk=archer::pmk_from_credential_pipe(fd);archer::SecretBytes<32> comparison_pmk;
  if(argc==3)std::copy(pmk.bytes.begin(),pmk.bytes.end(),comparison_pmk.bytes.begin());
  const fs::path dir=argv[1];
  unsigned m4_index=3,data_index=4,release_index=10;bool request_retry_layout=false;
  if(fs::exists(dir/"secure-layout.txt")){auto bytes=read_capture(dir/"secure-layout.txt");const std::string layout(bytes.begin(),bytes.end());request_retry_layout=layout=="wpa2-phase-layout-v2\n2 6 7 9 10 11 12\n";if(request_retry_layout)release_index=11;if(!request_retry_layout&&layout!="wpa2-phase-layout-v1\n2 6 7\n")throw std::runtime_error("Unsupported capture phase layout");m4_index=6;data_index=7;}
  auto m2_packet=read_capture(dir/"secure-tx-2.bin"),m4_packet=read_capture(dir/("secure-tx-"+std::to_string(m4_index)+".bin")),data_packet=read_capture(dir/("secure-tx-"+std::to_string(data_index)+".bin"));
  if(m2_packet.size()!=217||m4_packet.size()!=195||data_packet.size()<104)throw std::runtime_error("Unexpected recorded TX extent");
  std::vector<std::uint8_t> m2(m2_packet.begin()+96,m2_packet.end()),m4(m4_packet.begin()+96,m4_packet.end());
  std::array<std::uint8_t,32> snonce{};std::copy_n(m2.begin()+17,32,snonce.begin());
  archer::Wpa2PskHandshake hs(std::move(pmk),snonce);
  std::map<unsigned,fs::path> files;const std::regex pattern("rx-([0-9]+)\\.bin");
  for(auto& entry:fs::directory_iterator(dir)){auto n=entry.path().filename().string();std::smatch m;if(std::regex_match(n,m,pattern)){auto i=std::stoul(m[1]);if(i>=4096||!files.emplace(i,entry.path()).second)throw std::runtime_error("Invalid capture index");}}
  std::vector<std::uint8_t> first_m1,first_m3;std::vector<std::vector<std::uint8_t>> protected_frames;unsigned index=0;
  for(auto& [i,path]:files){if(i!=index++)throw std::runtime_error("Missing capture transfer");auto bytes=read_capture(path);for(auto& meta:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
   if(!meta.normal_80211_candidate())continue;const auto* p=bytes.data()+meta.packet.payload_offset;auto size=meta.packet.payload_bytes;
   if(first_m1.empty()&&archer::decode_target_eapol_m1(p,size))first_m1.assign(p,p+size);
   if(first_m3.empty()){auto e=archer::target_rsn_eapol(p,size);if(e&&archer::eapol_be16(e->data()+5)==0x13ca)first_m3=*e;}
   if(size>=44&&(archer::management_u16(p)&0x430c)==0x4208&&std::equal(archer::station_target_ap.begin(),archer::station_target_ap.end(),p+10)&&((p[4]&1)||std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),p+4)))protected_frames.emplace_back(p,p+size);
  }}
  hs.accept_m1(first_m1.data(),first_m1.size());if(hs.issue_m2()!=m2)throw std::runtime_error("Credential does not reproduce recorded M2; no plaintext examined");
  hs.accept_m3(first_m3);if(hs.issue_m4()!=m4)throw std::runtime_error("Recorded M4 mismatch");
  const auto* tx=data_packet.data()+64;const auto txsize=data_packet.size()-64;
  if(txsize<40)throw std::runtime_error("Short encrypted TX");
  const auto* iv=tx+24;const std::uint64_t pn=std::uint64_t(iv[0])|(std::uint64_t(iv[1])<<8)|(std::uint64_t(iv[4])<<16)|(std::uint64_t(iv[5])<<24)|(std::uint64_t(iv[6])<<32)|(std::uint64_t(iv[7])<<40);
  const auto params=archer::ccmp_parameters(tx,txsize,pn);auto txplain=archer::ccm8_decrypt(hs.pairwise_key(),params.nonce,params.aad,iv+8,txsize-32);
  if(txplain.bytes.size()<48)throw std::runtime_error("Recorded DHCP payload too short");const auto xid=archer::network_u32(txplain.bytes.data()+40);
  const auto flags=archer::eapol_be16(txplain.bytes.data()+46);
  if((flags!=0&&flags!=0x8000)||txplain.bytes!=archer::dhcp_discover_msdu(xid,flags==0x8000))throw std::runtime_error("Recorded encrypted DISCOVER does not reproduce expected payload");
  archer::CcmpSession ccmp(hs);unsigned accepted=0,rejected=0,pairwise=0,group=0,offers=0,reflected=0,acks=0;
  std::optional<archer::DhcpOffer> selected,acknowledged;std::array<std::uint8_t,6> ack_source{};
  for(auto& frame:protected_frames){
   try{
    auto plain=ccmp.decrypt_from_ap(frame.data(),frame.size());++accepted;const bool broadcast=frame[4]&1;if(broadcast)++group;else ++pairwise;
    const auto& b=plain.bytes;unsigned ether=b.size()>=8?archer::eapol_be16(b.data()+6):0,message=0;bool xid_match=false,chaddr_match=false;unsigned sport=0,dport=0;
    if(ether==0x800&&b.size()>=36){const auto* ip=b.data()+8;const unsigned ihl=(ip[0]&15)*4;if(ip[0]>>4==4&&ihl>=20&&b.size()>=8+ihl+8&&ip[9]==17){const auto* udp=ip+ihl;sport=archer::eapol_be16(udp);dport=archer::eapol_be16(udp+2);const auto n=archer::eapol_be16(udp+4);if(n>=248&&b.size()>=8+ihl+n){const auto* boot=udp+8;xid_match=archer::network_u32(boot+4)==xid;chaddr_match=std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),boot+28);for(unsigned at=240;at<n-8;){unsigned id=boot[at++];if(!id)continue;if(id==255||at>=n-8)break;unsigned len=boot[at++];if(len>n-8-at)break;if(id==53&&len==1)message=boot[at];at+=len;}}}}
    unsigned arp_operation=0;bool arp_sender_zero=false,arp_target_zero=false,arp_sender_ap=false,arp_target_station=false;
    if(ether==0x806&&b.size()==36&&archer::eapol_be16(b.data()+8)==1&&archer::eapol_be16(b.data()+10)==0x800&&b[12]==6&&b[13]==4){
     arp_operation=archer::eapol_be16(b.data()+14);
     arp_sender_zero=archer::network_u32(b.data()+22)==0;arp_target_zero=archer::network_u32(b.data()+32)==0;
     arp_sender_ap=std::equal(archer::station_target_ap.begin(),archer::station_target_ap.end(),b.begin()+16);
     arp_target_station=std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),b.begin()+26);
    }
    const bool echo=b==txplain.bytes&&std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),frame.begin()+16);reflected+=echo;
    const auto decoded_offer=archer::decode_dhcp_offer(b,xid);const bool offer=bool(decoded_offer);offers+=offer;if(decoded_offer&&!selected)selected=decoded_offer;
    const auto decoded_ack=archer::decode_dhcp_reply(b,xid,5);
    const bool ack=decoded_ack&&selected&&decoded_ack->address==selected->address&&decoded_ack->server==selected->server;
    if(ack){++acks;if(!acknowledged){acknowledged=decoded_ack;std::copy_n(frame.begin()+16,6,ack_source.begin());}}
    std::cout<<"{\"operation\":\"offline_authenticated_data\",\"group\":"<<(broadcast?"true":"false")<<",\"bytes\":"<<b.size()<<",\"ether_type\":"<<ether<<",\"udp_source\":"<<sport<<",\"udp_destination\":"<<dport<<",\"dhcp_message_type\":"<<message<<",\"xid_matches\":"<<(xid_match?"true":"false")<<",\"chaddr_matches\":"<<(chaddr_match?"true":"false")<<",\"arp_operation\":"<<arp_operation<<",\"arp_sender_ip_zero\":"<<(arp_sender_zero?"true":"false")<<",\"arp_target_ip_zero\":"<<(arp_target_zero?"true":"false")<<",\"arp_sender_is_ap\":"<<(arp_sender_ap?"true":"false")<<",\"arp_target_is_station\":"<<(arp_target_station?"true":"false")<<",\"matches_recorded_discover\":"<<(echo?"true":"false")<<",\"strict_offer\":"<<(offer?"true":"false")<<",\"strict_ack\":"<<(ack?"true":"false")<<"}"<<std::endl;
   }catch(const std::exception&){++rejected;}
  }
  bool request_verified=false,release_verified=false,request_retry_verified=false;std::uint64_t request_pn=0;
  const auto verify_tx=[&](const fs::path& path,std::uint64_t& packet_pn){
   auto packet=read_capture(path);if(packet.size()!=440)throw std::runtime_error("Unexpected recorded data TX extent");
   const auto* frame=packet.data()+64;const auto size=packet.size()-64;
   if(archer::management_u16(frame)!=0x4108||!std::equal(archer::station_target_ap.begin(),archer::station_target_ap.end(),frame+4)||!std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),frame+10))throw std::runtime_error("Recorded data TX identity mismatch");
   const auto* v=frame+24;if(v[2]||v[3]!=0x20)throw std::runtime_error("Recorded TX CCMP key ID mismatch");packet_pn=std::uint64_t(v[0])|(std::uint64_t(v[1])<<8)|(std::uint64_t(v[4])<<16)|(std::uint64_t(v[5])<<24)|(std::uint64_t(v[6])<<32)|(std::uint64_t(v[7])<<40);
   const auto parameters=archer::ccmp_parameters(frame,size,packet_pn);
   return archer::ccm8_decrypt(hs.pairwise_key(),parameters.nonce,parameters.aad,v+8,size-32);
  };
  if(m4_index==6&&fs::exists(dir/"secure-tx-9.bin")){
   if(!selected)throw std::runtime_error("Recorded REQUEST without matching OFFER");
   const auto request=verify_tx(dir/"secure-tx-9.bin",request_pn);const auto seconds=archer::eapol_be16(request.bytes.data()+44);
   const auto request_packet=read_capture(dir/"secure-tx-9.bin");
   if(request_pn<=pn||!std::all_of(request_packet.begin()+80,request_packet.begin()+86,[](auto b){return b==255;})||request.bytes!=archer::dhcp_selecting_request(xid,*selected,seconds))throw std::runtime_error("Recorded REQUEST does not reproduce selected offer");request_verified=true;
  }
  if(request_retry_layout&&fs::exists(dir/"secure-tx-10.bin")){
   if(!request_verified||!selected)throw std::runtime_error("Recorded REQUEST retry without selected original");
   std::uint64_t retry_pn=0;const auto retry=verify_tx(dir/"secure-tx-10.bin",retry_pn);const auto packet=read_capture(dir/"secure-tx-10.bin");
   const auto original=verify_tx(dir/"secure-tx-9.bin",request_pn);
   if(retry_pn<=request_pn||archer::eapol_be16(retry.bytes.data()+44)<archer::eapol_be16(original.bytes.data()+44)||retry.bytes!=archer::dhcp_selecting_request(xid,*selected,archer::eapol_be16(retry.bytes.data()+44))||!std::all_of(packet.begin()+80,packet.begin()+86,[](auto b){return b==255;}))throw std::runtime_error("Recorded REQUEST retry mismatch");
   request_pn=retry_pn;request_retry_verified=true;
  }
  if(m4_index==6&&fs::exists(dir/("secure-tx-"+std::to_string(release_index)+".bin"))){
   if(!request_verified||!acknowledged)throw std::runtime_error("Recorded RELEASE without matching ACK");
   std::uint64_t release_pn=0;const auto release=verify_tx(dir/("secure-tx-"+std::to_string(release_index)+".bin"),release_pn);const auto packet=read_capture(dir/("secure-tx-"+std::to_string(release_index)+".bin"));
   if(release_pn<=request_pn||release.bytes!=archer::dhcp_release_msdu(xid,*acknowledged)||!std::equal(ack_source.begin(),ack_source.end(),packet.begin()+80))throw std::runtime_error("Recorded RELEASE does not reproduce ACK lease/source");release_verified=true;
  }
  if(argc==3){
   const fs::path attempt=argv[2];const auto packet=read_capture(attempt/"secure-tx-2.bin");
   if(packet.size()!=217)throw std::runtime_error("Unexpected comparison M2 extent");
   std::vector<std::uint8_t> expected(packet.begin()+96,packet.end()),m1;
   std::array<std::uint8_t,32> attempt_nonce{};std::copy_n(expected.begin()+17,32,attempt_nonce.begin());
   std::map<unsigned,fs::path> attempt_files;
   for(auto& entry:fs::directory_iterator(attempt)){auto n=entry.path().filename().string();std::smatch m;if(std::regex_match(n,m,pattern)){auto i=std::stoul(m[1]);if(i>=4096||!attempt_files.emplace(i,entry.path()).second)throw std::runtime_error("Invalid comparison index");}}
   unsigned next=0;
   for(auto& [i,path]:attempt_files){if(i!=next++)throw std::runtime_error("Missing comparison transfer");auto bytes=read_capture(path);
    for(auto& meta:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
     if(!meta.normal_80211_candidate())continue;const auto* frame=bytes.data()+meta.packet.payload_offset;const auto size=meta.packet.payload_bytes;
     if(archer::decode_target_eapol_m1(frame,size)){m1.assign(frame,frame+size);break;}
    }
    if(!m1.empty())break;
   }
   if(m1.empty())throw std::runtime_error("Comparison capture has no valid M1");
   archer::Wpa2PskHandshake attempt_hs(std::move(comparison_pmk),attempt_nonce);attempt_hs.accept_m1(m1.data(),m1.size());
   const bool reproduced=attempt_hs.issue_m2()==expected;
   std::cout<<"{\"operation\":\"offline_credential_comparison\",\"reference_m3_authenticated\":true,\"attempt_m2_reproduced\":"<<(reproduced?"true":"false")<<",\"h2c_submissions\":0,\"network_submissions\":0}"<<std::endl;
  }
  std::cout<<"{\"result\":\"offline_wpa2_audited\",\"m2_reproduced\":true,\"m3_authenticated\":true,\"m4_reproduced\":true,\"recorded_discover_verified\":true,\"protected_frames\":"<<protected_frames.size()<<",\"authenticated_frames\":"<<accepted<<",\"rejected_frames\":"<<rejected<<",\"pairwise_frames\":"<<pairwise<<",\"group_frames\":"<<group<<",\"matched_acks\":"<<acks<<",\"recorded_request_verified\":"<<(request_verified?"true":"false")<<",\"recorded_release_verified\":"<<(release_verified?"true":"false")<<",\"recorded_request_retry_verified\":"<<(request_retry_verified?"true":"false")<<",\"server_release_confirmed\":false,\"matched_offers\":"<<offers<<",\"reflected_discover_frames\":"<<reflected<<",\"h2c_submissions\":0,\"network_submissions\":0}"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"Offline audit: "<<e.what()<<'\n';return 2;}
}
