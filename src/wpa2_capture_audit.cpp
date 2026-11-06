#include "archer/secure_network.hpp"
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
 if(argc!=2){std::cerr<<"Usage: wpa2-capture-audit CAPTURE_DIRECTORY (private credential FD required)\n";return 2;}
 try{
  const auto value=std::getenv("ARCHER_CREDENTIAL_FD");char* end=nullptr;errno=0;long fd=value?std::strtol(value,&end,10):-1;
  if(!value||errno||!end||*end||fd<3||fd>0x7fffffff)throw std::runtime_error("Private credential FD required");
  auto pmk=archer::pmk_from_credential_pipe(fd);const fs::path dir=argv[1];
  auto m2_packet=read_capture(dir/"secure-tx-2.bin"),m4_packet=read_capture(dir/"secure-tx-3.bin"),data_packet=read_capture(dir/"secure-tx-4.bin");
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
  archer::CcmpSession ccmp(hs);unsigned accepted=0,rejected=0,pairwise=0,group=0,offers=0,reflected=0;
  for(auto& frame:protected_frames){
   try{
    auto plain=ccmp.decrypt_from_ap(frame.data(),frame.size());++accepted;const bool broadcast=frame[4]&1;if(broadcast)++group;else ++pairwise;
    const auto& b=plain.bytes;unsigned ether=b.size()>=8?archer::eapol_be16(b.data()+6):0,message=0;bool xid_match=false,chaddr_match=false;unsigned sport=0,dport=0;
    if(ether==0x800&&b.size()>=36){const auto* ip=b.data()+8;const unsigned ihl=(ip[0]&15)*4;if(ip[0]>>4==4&&ihl>=20&&b.size()>=8+ihl+8&&ip[9]==17){const auto* udp=ip+ihl;sport=archer::eapol_be16(udp);dport=archer::eapol_be16(udp+2);const auto n=archer::eapol_be16(udp+4);if(n>=248&&b.size()>=8+ihl+n){const auto* boot=udp+8;xid_match=archer::network_u32(boot+4)==xid;chaddr_match=std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),boot+28);for(unsigned at=240;at<n-8;){unsigned id=boot[at++];if(!id)continue;if(id==255||at>=n-8)break;unsigned len=boot[at++];if(len>n-8-at)break;if(id==53&&len==1)message=boot[at];at+=len;}}}}
    const bool echo=b==txplain.bytes&&std::equal(archer::station_test_mac.begin(),archer::station_test_mac.end(),frame.begin()+16);reflected+=echo;
    const bool offer=bool(archer::decode_dhcp_offer(b,xid));offers+=offer;
    std::cout<<"{\"operation\":\"offline_authenticated_data\",\"group\":"<<(broadcast?"true":"false")<<",\"bytes\":"<<b.size()<<",\"ether_type\":"<<ether<<",\"udp_source\":"<<sport<<",\"udp_destination\":"<<dport<<",\"dhcp_message_type\":"<<message<<",\"xid_matches\":"<<(xid_match?"true":"false")<<",\"chaddr_matches\":"<<(chaddr_match?"true":"false")<<",\"matches_recorded_discover\":"<<(echo?"true":"false")<<",\"strict_offer\":"<<(offer?"true":"false")<<"}"<<std::endl;
   }catch(const std::exception&){++rejected;}
  }
  std::cout<<"{\"result\":\"offline_wpa2_audited\",\"m2_reproduced\":true,\"m3_authenticated\":true,\"m4_reproduced\":true,\"recorded_discover_verified\":true,\"protected_frames\":"<<protected_frames.size()<<",\"authenticated_frames\":"<<accepted<<",\"rejected_frames\":"<<rejected<<",\"pairwise_frames\":"<<pairwise<<",\"group_frames\":"<<group<<",\"matched_offers\":"<<offers<<",\"reflected_discover_frames\":"<<reflected<<",\"h2c_submissions\":0,\"network_submissions\":0}"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"Offline audit: "<<e.what()<<'\n';return 2;}
}
