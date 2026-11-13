#include "archer/dhcp_selecting.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
std::vector<std::uint8_t> load(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(){
 static_assert(unsigned(SecureFramePhase::deauthenticate)==8);
 std::array<std::uint8_t,32> nonce{};for(unsigned i=0;i<32;++i)nonce[i]=i+1;
 Wpa2PskHandshake hs(derive_wpa2_pmk("password","Especial"),nonce);auto m1=load("tests/fixtures/special-eapol-m1-stage104.bin");hs.accept_m1(m1.data(),m1.size());auto m2=hs.issue_m2();hs.accept_m3(load("tests/fixtures/wpa2-test-m3.bin"));auto m4=hs.issue_m4();
 CcmpSession ccmp(hs);SecureFrameSendGuard guard;
 for(auto phase:{SecureFramePhase::authenticate,SecureFramePhase::associate})guard.issue(phase,secure_management_packet(phase));
 guard.issue(SecureFramePhase::m2,secure_eapol_packet(SecureFramePhase::m2,m2));guard.issue(SecureFramePhase::m4,secure_eapol_packet(SecureFramePhase::m4,m4));
 const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};DhcpOffer offer;offer.address={192,0,2,10};offer.server={192,0,2,1};offer.lease_seconds=3600;
 const auto data=[&](SecureFramePhase phase,const std::vector<std::uint8_t>& msdu){return secure_tx_packet(phase,ccmp.encrypt_to_ap(msdu,broadcast,secure_frame_sequence(phase)));};
 auto discover=data(SecureFramePhase::discover,dhcp_discover_msdu(1,true));guard.issue(SecureFramePhase::discover,discover);
 auto release=data(SecureFramePhase::release,dhcp_release_msdu(1,offer));rejects([&]{guard.issue(SecureFramePhase::release,release);});
 auto request=data(SecureFramePhase::request,dhcp_selecting_request(1,offer));guard.issue(SecureFramePhase::request,request);rejects([&]{guard.issue(SecureFramePhase::request,request);});
 assert(channel_u32(release.data()+36)==(secure_release_tag<<28)&&channel_u32(release.data()+40)==0x40000000);
 assert(channel_u32(request.data()+36)==0&&channel_u32(request.data()+40)==0);
 guard.issue(SecureFramePhase::release,release);rejects([&]{guard.issue(SecureFramePhase::release,release);});
 auto deauth=secure_management_packet(SecureFramePhase::deauthenticate);assert(channel_u32(deauth.data()+36)==(secure_cleanup_tag<<28));guard.issue(SecureFramePhase::deauthenticate,deauth);
 rejects([&]{guard.issue(SecureFramePhase::deauthenticate,deauth);});
 SecureFrameSendGuard retried;
 for(auto phase:{SecureFramePhase::authenticate,SecureFramePhase::associate})retried.issue(phase,secure_management_packet(phase));
 retried.issue(SecureFramePhase::m2,secure_eapol_packet(SecureFramePhase::m2,m2));retried.issue(SecureFramePhase::m4,secure_eapol_packet(SecureFramePhase::m4,m4));retried.issue(SecureFramePhase::discover,discover);
 auto retry=data(SecureFramePhase::discover_retry,dhcp_discover_msdu(1,true,4));assert(retry[88]>discover[88]);assert(management_u16(retry.data()+86)!=management_u16(discover.data()+86));retried.issue(SecureFramePhase::discover_retry,retry);rejects([&]{retried.issue(SecureFramePhase::discover_retry,retry);});retried.issue(SecureFramePhase::request,request);retried.issue(SecureFramePhase::release,release);retried.issue(SecureFramePhase::deauthenticate,deauth);
 SecureFrameSendGuard early;early.issue(SecureFramePhase::authenticate,secure_management_packet(SecureFramePhase::authenticate));early.issue(SecureFramePhase::deauthenticate,deauth);rejects([&]{early.issue(SecureFramePhase::associate,secure_management_packet(SecureFramePhase::associate));});
 std::cout<<"Eight-phase lease descriptors, distinct reports, one-shot ordering and early deauth passed\n";
}
