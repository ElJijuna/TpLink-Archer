#include "archer/secure_network.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(){
 SecureFrameSendGuard guard;
 const auto auth=secure_management_packet(SecureFramePhase::authenticate),assoc=secure_management_packet(SecureFramePhase::associate),deauth=secure_management_packet(SecureFramePhase::deauthenticate);
 assert(channel_u32(deauth.data()+36)==(secure_cleanup_tag<<28));
 assert(auth.size()==94&&assoc.size()==130&&deauth.size()==90);
 assert(management_u16(auth.data()+86)==secure_first_sequence*16&&management_u16(deauth.data()+86)==(secure_first_sequence+5)*16);
 rejects([&]{guard.issue(SecureFramePhase::associate,assoc);});
 guard.issue(SecureFramePhase::authenticate,auth);rejects([&]{guard.issue(SecureFramePhase::authenticate,auth);});guard.issue(SecureFramePhase::associate,assoc);
 auto m2=secure_eapol_packet(SecureFramePhase::m2,read("tests/fixtures/wpa2-test-m2.bin"));
 auto m4=secure_eapol_packet(SecureFramePhase::m4,read("tests/fixtures/wpa2-test-m4.bin"));
 assert(channel_u32(m2.data()+12)==secure_frame_sequence(SecureFramePhase::m2));
 assert(channel_u32(auth.data()+12)==(0x2000|secure_frame_sequence(SecureFramePhase::authenticate)));
 assert(channel_u32(m2.data())==0x00406480&&channel_u32(m2.data()+40)==0&&channel_u32(deauth.data()+40)==0x40000000);
 auto changed=m2;changed[40]^=1;rejects([&]{guard.issue(SecureFramePhase::m2,changed);});guard.issue(SecureFramePhase::m2,m2);guard.issue(SecureFramePhase::m4,m4);
 std::array<std::uint8_t,32> snonce{};for(unsigned i=0;i<32;++i)snonce[i]=i+1;Wpa2PskHandshake hs(derive_wpa2_pmk("password","Especial"),snonce);auto raw_m1=read("tests/fixtures/special-eapol-m1-stage104.bin");hs.accept_m1(raw_m1.data(),raw_m1.size());hs.issue_m2();hs.accept_m3(read("tests/fixtures/wpa2-test-m3.bin"));hs.issue_m4();CcmpSession session(hs);const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};auto data=secure_tx_packet(SecureFramePhase::discover,session.encrypt_to_ap(dhcp_discover_msdu(0x12345678),broadcast,secure_frame_sequence(SecureFramePhase::discover)));assert(data.size()==440);guard.issue(SecureFramePhase::discover,data);rejects([&]{guard.issue(SecureFramePhase::discover,data);});
 guard.issue(SecureFramePhase::deauthenticate,deauth);rejects([&]{guard.issue(SecureFramePhase::deauthenticate,deauth);});
 SecureFrameSendGuard early;early.issue(SecureFramePhase::authenticate,auth);early.issue(SecureFramePhase::deauthenticate,deauth);rejects([&]{early.issue(SecureFramePhase::associate,assoc);});
 auto m1=read("tests/fixtures/special-eapol-m1-stage104.bin");auto e=target_rsn_eapol(m1.data(),m1.size());assert(e&&e->size()==99);for(std::size_t n=0;n<m1.size();++n)assert(!target_rsn_eapol(m1.data(),n));
 const unsigned xid=0x12345678;auto discover=dhcp_discover_msdu(xid);assert(discover.size()==336&&internet_checksum(discover.data()+8,20)==0);assert(!decode_dhcp_offer(discover,xid));rejects([]{dhcp_discover_msdu(0);});
 // Construct a server OFFER independently at the BOOTP/IP boundaries.
 auto offer=discover;auto* ip=offer.data()+8;auto* udp=ip+20;auto* b=udp+8;b[0]=2;b[16]=192;b[17]=168;b[18]=1;b[19]=55;
 b[240]=53;b[241]=1;b[242]=2;b[243]=54;b[244]=4;b[245]=192;b[246]=168;b[247]=1;b[248]=1;b[249]=255;
 eapol_put_be16(udp,67);eapol_put_be16(udp+2,68);ip[12]=192;ip[13]=168;ip[14]=1;ip[15]=1;ip[10]=ip[11]=0;eapol_put_be16(ip+10,internet_checksum(ip,20));
 auto decoded=decode_dhcp_offer(offer,xid);assert(decoded&&decoded->address[3]==55&&decoded->server[3]==1);assert(!decode_dhcp_offer(offer,xid+1));
 for(unsigned at:{8u,18u,28u,36u,40u,64u,272u,280u}){auto bad=offer;bad[at]^=1;assert(!decode_dhcp_offer(bad,xid));}
 for(std::size_t n=0;n<offer.size();++n){auto short_offer=offer;short_offer.resize(n);assert(!decode_dhcp_offer(short_offer,xid));}
 // Exercise the same authenticated observation seam used by the live RX callback.
 const auto downlink=[&](const std::vector<std::uint8_t>& payload,bool group,unsigned pn,bool own_source=true){
  std::vector<std::uint8_t> f(24);f[0]=8;f[1]=0x42;
  std::copy(station_target_ap.begin(),station_target_ap.end(),f.begin()+10);
  std::copy(station_test_mac.begin(),station_test_mac.end(),f.begin()+4);
  if(group)std::fill(f.begin()+4,f.begin()+10,255);
  const auto& source=own_source?station_test_mac:station_target_ap;std::copy(source.begin(),source.end(),f.begin()+16);
  auto params=ccmp_parameters(f.data(),f.size(),pn);
  auto cipher=ccm8_encrypt(group?hs.group_key():hs.pairwise_key(),params.nonce,params.aad,payload);
  f.insert(f.end(),{std::uint8_t(pn),std::uint8_t(pn>>8),0,std::uint8_t(group?0x60:0x20),0,0,0,0});
  f.insert(f.end(),cipher.begin(),cipher.end());auto n=f.size();f.resize(n+4);channel_put32(f.data()+n,ieee_fcs(f.data(),n));return f;
 };
 CcmpSession observed_session(hs);SecureDataObservation observed;
 assert(!observed.observe(observed_session,m1.data(),m1.size(),xid));assert(observed.rejected==0);
 auto echo=downlink(discover,true,1000);auto corrupt=echo;corrupt[40]^=1;channel_put32(corrupt.data()+corrupt.size()-4,ieee_fcs(corrupt.data(),corrupt.size()-4));
 assert(!observed.observe(observed_session,corrupt.data(),corrupt.size(),xid));assert(observed.rejected==1);
 auto reflected=observed.observe(observed_session,echo.data(),echo.size(),xid);
 assert(reflected&&reflected->group&&reflected->reflected_discover&&!reflected->offer&&reflected->ether_type==0x800);
 assert(!observed.observe(observed_session,echo.data(),echo.size(),xid)); // replay never increments authenticated
 auto other_source=downlink(discover,true,1001,false);auto other=observed.observe(observed_session,other_source.data(),other_source.size(),xid);
 assert(other&&!other->reflected_discover&&!other->offer);
 auto wrong_transaction=downlink(dhcp_discover_msdu(xid+1),true,1002);auto wrong=observed.observe(observed_session,wrong_transaction.data(),wrong_transaction.size(),xid);
 assert(wrong&&!wrong->reflected_discover&&!wrong->offer);
 auto server=downlink(offer,false,1,false);auto accepted=observed.observe(observed_session,server.data(),server.size(),xid);
 assert(accepted&&!accepted->group&&accepted->offer&&!accepted->reflected_discover);
 assert(observed.authenticated==4&&observed.rejected==2&&observed.group==3&&observed.pairwise==1&&observed.reflected_discover==1&&observed.offers==1);
 std::cout<<"Secure frame one-shot ordering/descriptors, captured EAPOL extraction and matched DHCP OFFER bounds passed\n";
}
