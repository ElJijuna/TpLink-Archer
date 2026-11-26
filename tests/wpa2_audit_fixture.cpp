#include "archer/unicast_icmp.hpp"
#include <filesystem>
#include <fstream>
#include <cassert>
namespace fs=std::filesystem;
using namespace archer;
std::vector<std::uint8_t> load(const char* p){std::ifstream f(p,std::ios::binary);assert(f);return {(std::istreambuf_iterator<char>(f)),{}};}
void save(const fs::path& p,const std::vector<std::uint8_t>& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f);}
void fcs(std::vector<std::uint8_t>& b){auto n=b.size();b.resize(n+4);channel_put32(b.data()+n,ieee_fcs(b.data(),n));}
std::vector<std::uint8_t> header(unsigned fc){std::vector<std::uint8_t> b(24);b[0]=fc;b[1]=fc>>8;std::copy(station_test_mac.begin(),station_test_mac.end(),b.begin()+4);std::copy(station_target_ap.begin(),station_target_ap.end(),b.begin()+10);std::copy(station_target_ap.begin(),station_target_ap.end(),b.begin()+16);return b;}
void rx(const fs::path& dir,unsigned n,const std::vector<std::uint8_t>& frame){std::vector<std::uint8_t> b(40);channel_put32(b.data(),0x80000000|frame.size());b.insert(b.end(),frame.begin(),frame.end());while(b.size()%8)b.push_back(0);save(dir/("rx-"+std::to_string(n)+".bin"),b);}
int main(int argc,char** argv){if(argc!=2&&argc!=3)return 2;const bool broadcast_reply=argc==3;fs::path dir=argv[1];assert(fs::create_directory(dir));std::array<std::uint8_t,32> snonce{};for(unsigned i=0;i<32;++i)snonce[i]=i+1;auto m1=load("tests/fixtures/special-eapol-m1-stage104.bin"),m3=load("tests/fixtures/wpa2-test-m3.bin");Wpa2PskHandshake hs(derive_wpa2_pmk("password","Especial"),snonce);hs.accept_m1(m1.data(),m1.size());auto m2=hs.issue_m2();hs.accept_m3(m3);auto m4=hs.issue_m4();save(dir/"secure-tx-2.bin",secure_eapol_packet(SecureFramePhase::m2,m2));save(dir/"secure-tx-3.bin",secure_eapol_packet(SecureFramePhase::m4,m4));CcmpSession session(hs);auto discover=dhcp_discover_msdu(0x12345678,broadcast_reply);const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};save(dir/"secure-tx-4.bin",secure_tx_packet(SecureFramePhase::discover,session.encrypt_to_ap(discover,broadcast,secure_frame_sequence(SecureFramePhase::discover))));rx(dir,0,m1);auto m3f=header(0x0208);m3f.insert(m3f.end(),{0xaa,0xaa,3,0,0,0,0x88,0x8e});m3f.insert(m3f.end(),m3.begin(),m3.end());fcs(m3f);rx(dir,1,m3f);
 auto offer=discover;auto* ip=offer.data()+8;auto* udp=ip+20;auto* boot=udp+8;boot[0]=2;boot[16]=192;boot[17]=168;boot[18]=1;boot[19]=55;boot[242]=2;boot[243]=54;boot[244]=4;boot[245]=192;boot[246]=168;boot[247]=1;boot[248]=1;boot[249]=255;ip[12]=192;ip[13]=168;ip[14]=1;ip[15]=1;ip[10]=ip[11]=0;eapol_put_be16(ip+10,internet_checksum(ip,20));eapol_put_be16(udp,67);eapol_put_be16(udp+2,68);auto frame=header(0x4208);auto params=ccmp_parameters(frame.data(),frame.size(),1);auto encrypted=ccm8_encrypt(hs.pairwise_key(),params.nonce,params.aad,offer);frame.insert(frame.end(),{1,0,0,0x20,0,0,0,0});frame.insert(frame.end(),encrypted.begin(),encrypted.end());fcs(frame);rx(dir,2,frame);
 auto echo=header(0x4208);std::fill(echo.begin()+4,echo.begin()+10,255);std::copy(station_test_mac.begin(),station_test_mac.end(),echo.begin()+16);auto gp=ccmp_parameters(echo.data(),echo.size(),1);auto gc=ccm8_encrypt(hs.group_key(),gp.nonce,gp.aad,discover);echo.insert(echo.end(),{1,0,0,0x60,0,0,0,0});echo.insert(echo.end(),gc.begin(),gc.end());fcs(echo);rx(dir,3,echo);
 // Synthetic ARP request, sender IPv4 zero; only metadata may leave the auditor.
 std::vector<std::uint8_t> arp{0xaa,0xaa,3,0,0,0,8,6,0,1,8,0,6,4,0,1};
 arp.insert(arp.end(),station_target_ap.begin(),station_target_ap.end());arp.insert(arp.end(),4,0);
 arp.insert(arp.end(),station_test_mac.begin(),station_test_mac.end());arp.insert(arp.end(),{192,0,2,10});
 auto af=header(0x4208);std::fill(af.begin()+4,af.begin()+10,255);auto ap=ccmp_parameters(af.data(),af.size(),2);auto ac=ccm8_encrypt(hs.group_key(),ap.nonce,ap.aad,arp);
 af.insert(af.end(),{2,0,0,0x60,0,0,0,0});af.insert(af.end(),ac.begin(),ac.end());fcs(af);rx(dir,4,af);
#ifdef ARCHER_EAPOL_RETRY
 if(argc==3&&std::string(argv[2])=="lease"){
  fs::rename(dir/"secure-tx-3.bin",dir/"secure-tx-6.bin");fs::rename(dir/"secure-tx-4.bin",dir/"secure-tx-7.bin");
  {std::ofstream layout(dir/"secure-layout.txt");
#ifdef ARCHER_UNICAST_ICMP
  layout<<"wpa2-phase-layout-v4\n2 6 7 9 10 11 12 13 14\n";
#elif defined(ARCHER_UNICAST_ARP)
  layout<<"wpa2-phase-layout-v3\n2 6 7 9 10 11 12 13\n";
#else
  layout<<"wpa2-phase-layout-v2\n2 6 7 9 10 11 12\n";
#endif
  }
  auto ack=offer;auto* a=ack.data()+36;a[242]=5;a[249]=51;a[250]=4;network_put32(a+251,3600);a[255]=255;
#ifdef ARCHER_UNICAST_ARP
  a[255]=1;a[256]=4;a[257]=255;a[258]=255;a[259]=255;a[260]=0;
  a[261]=3;a[262]=4;a[263]=192;a[264]=168;a[265]=1;a[266]=1;a[267]=255;
#endif
  auto ack_frame=header(0x4208);std::fill(ack_frame.begin()+4,ack_frame.begin()+10,255);auto ack_params=ccmp_parameters(ack_frame.data(),ack_frame.size(),3);auto ack_cipher=ccm8_encrypt(hs.group_key(),ack_params.nonce,ack_params.aad,ack);
  ack_frame.insert(ack_frame.end(),{3,0,0,0x60,0,0,0,0});ack_frame.insert(ack_frame.end(),ack_cipher.begin(),ack_cipher.end());fcs(ack_frame);rx(dir,5,ack_frame);
  const auto selected=decode_dhcp_offer(offer,0x12345678),lease=decode_dhcp_reply(ack,0x12345678,5);assert(selected&&lease);
  auto request=session.encrypt_to_ap(dhcp_selecting_request(0x12345678,*selected,4),broadcast,secure_frame_sequence(SecureFramePhase::request));save(dir/"secure-tx-9.bin",secure_tx_packet(SecureFramePhase::request,request));
  auto retried=session.encrypt_to_ap(dhcp_selecting_request(0x12345678,*selected,8),broadcast,secure_frame_sequence(SecureFramePhase::request_retry));save(dir/"secure-tx-10.bin",secure_tx_packet(SecureFramePhase::request_retry,retried));
#ifdef ARCHER_UNICAST_ARP
  GatewayArpProbe arp_probe(*lease);auto arp=arp_probe.issue_request();
  auto arp_tx=session.encrypt_to_ap(arp,broadcast,secure_frame_sequence(SecureFramePhase::arp_request));save(dir/"secure-tx-11.bin",secure_tx_packet(SecureFramePhase::arp_request,arp_tx));
  arp[15]=2;std::copy(station_target_ap.begin(),station_target_ap.end(),arp.begin()+16);std::copy(lease->router.begin(),lease->router.end(),arp.begin()+22);std::copy(station_test_mac.begin(),station_test_mac.end(),arp.begin()+26);std::copy(lease->address.begin(),lease->address.end(),arp.begin()+32);
  auto reply_frame=header(0x4208);const auto reply_params=ccmp_parameters(reply_frame.data(),reply_frame.size(),2);const auto reply_cipher=ccm8_encrypt(hs.pairwise_key(),reply_params.nonce,reply_params.aad,arp);
  reply_frame.insert(reply_frame.end(),{2,0,0,0x20,0,0,0,0});reply_frame.insert(reply_frame.end(),reply_cipher.begin(),reply_cipher.end());fcs(reply_frame);rx(dir,6,reply_frame);
#endif
#ifdef ARCHER_UNICAST_ICMP
  std::array<std::uint8_t,16> cookie{};for(unsigned i=0;i<16;++i)cookie[i]=i+1;
  GatewayIcmpProbe echo_probe(*lease,station_target_ap,123,cookie);auto echo=echo_probe.issue_request();
  auto echo_tx=session.encrypt_to_ap(echo,station_target_ap,secure_frame_sequence(SecureFramePhase::echo_request));save(dir/"secure-tx-12.bin",secure_tx_packet(SecureFramePhase::echo_request,echo_tx));
  std::copy(lease->router.begin(),lease->router.end(),echo.begin()+20);std::copy(lease->address.begin(),lease->address.end(),echo.begin()+24);echo[28]=0;echo[18]=echo[19]=echo[30]=echo[31]=0;eapol_put_be16(echo.data()+18,internet_checksum(echo.data()+8,20));eapol_put_be16(echo.data()+30,internet_checksum(echo.data()+28,24));
  auto echo_frame=header(0x4208);const auto echo_params=ccmp_parameters(echo_frame.data(),echo_frame.size(),3);const auto echo_cipher=ccm8_encrypt(hs.pairwise_key(),echo_params.nonce,echo_params.aad,echo);
  echo_frame.insert(echo_frame.end(),{3,0,0,0x20,0,0,0,0});echo_frame.insert(echo_frame.end(),echo_cipher.begin(),echo_cipher.end());fcs(echo_frame);rx(dir,7,echo_frame);
#endif
  auto released=session.encrypt_to_ap(dhcp_release_msdu(0x12345678,*lease),station_target_ap,secure_frame_sequence(SecureFramePhase::release));save(dir/("secure-tx-"+std::to_string(unsigned(SecureFramePhase::release))+".bin"),secure_tx_packet(SecureFramePhase::release,released));
 }
#endif
 return 0;}
