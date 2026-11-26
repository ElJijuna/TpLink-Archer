#include "archer/unicast_arp.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
std::vector<std::uint8_t> load(const char* p){std::ifstream f(p,std::ios::binary);assert(f);return {(std::istreambuf_iterator<char>(f)),{}};}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int main(){
#ifdef ARCHER_UNICAST_ARP
 static_assert(unsigned(SecureFramePhase::deauthenticate)==13);
 assert(secure_first_sequence==150&&secure_report_tag(SecureFramePhase::arp_request)==15&&secure_report_requested(SecureFramePhase::arp_request));
#else
 static_assert(unsigned(SecureFramePhase::deauthenticate)==12);
#endif
 assert(secure_report_requested(SecureFramePhase::discover));
 assert(secure_report_tag(SecureFramePhase::discover)==12);
 assert(!secure_report_requested(SecureFramePhase::discover_retry));
 rejects([]{require_secure_discover_report(std::nullopt);});
 rejects([]{require_secure_discover_report(ProbeTxReport{1,1});});
 rejects([]{require_secure_discover_report(ProbeTxReport{0,2});});
 require_secure_discover_report(ProbeTxReport{0,1});
 for(unsigned replies=0;replies<=3;++replies)for(bool dhcp_retry:{false,true})for(bool request_retry:{false,true})
#ifdef ARCHER_UNICAST_ARP
 for(bool arp_sent:{false,true})
#endif
 {
  std::array<std::uint8_t,32> nonce{};for(unsigned i=0;i<32;++i)nonce[i]=i+1;
  auto m1=load("tests/fixtures/special-eapol-m1-stage104.bin");auto decoded=decode_target_eapol_m1(m1.data(),m1.size());assert(decoded);
  auto pmk=derive_wpa2_pmk("password","Especial");auto ptk=derive_wpa2_ptk(pmk,decoded->nonce,nonce);
  Wpa2PskHandshake hs(std::move(pmk),nonce);hs.accept_m1(m1.data(),m1.size());auto m2=hs.issue_m2();
  SecureFrameSendGuard guard;unsigned last_sequence=0;
  const auto send=[&](SecureFramePhase phase,const std::vector<std::uint8_t>& packet){const auto seq=management_u16(packet.data()+86)>>4;assert(seq>last_sequence);last_sequence=seq;guard.issue(phase,packet);rejects([&]{guard.issue(phase,packet);});};
  send(SecureFramePhase::authenticate,secure_management_packet(SecureFramePhase::authenticate));send(SecureFramePhase::associate,secure_management_packet(SecureFramePhase::associate));send(SecureFramePhase::m2,secure_eapol_packet(SecureFramePhase::m2,m2));
  for(unsigned retry=0;retry<replies;++retry){
   auto frame=m1;const unsigned header=(frame[0]&0x80)?26:24;frame[header+8+16]=retry+2;channel_put32(frame.data()+frame.size()-4,ieee_fcs(frame.data(),frame.size()-4));
   const auto reply=hs.issue_m2_for_m1_retry(frame.data(),frame.size());const auto phase=static_cast<SecureFramePhase>(unsigned(SecureFramePhase::m2_retry1)+retry);
   send(phase,secure_eapol_packet(phase,reply));
  }
  auto m3=load("tests/fixtures/wpa2-test-m3.bin");m3[16]=5;sign_wpa2_eapol(m3,ptk);hs.accept_m3(m3);auto m4=hs.issue_m4();send(SecureFramePhase::m4,secure_eapol_packet(SecureFramePhase::m4,m4));
  CcmpSession ccmp(hs);const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};
  const auto data=[&](SecureFramePhase phase,const std::vector<std::uint8_t>& msdu){send(phase,secure_tx_packet(phase,ccmp.encrypt_to_ap(msdu,broadcast,secure_frame_sequence(phase))));};
  data(SecureFramePhase::discover,dhcp_discover_msdu(1,true));if(dhcp_retry)data(SecureFramePhase::discover_retry,dhcp_discover_msdu(1,true,4));
  DhcpOffer selected;selected.address={192,0,2,10};selected.server={192,0,2,1};selected.lease_seconds=60;
  data(SecureFramePhase::request,dhcp_selecting_request(1,selected));if(request_retry)data(SecureFramePhase::request_retry,dhcp_selecting_request(1,selected,8));
#ifdef ARCHER_UNICAST_ARP
  selected.router=selected.server;selected.subnet={255,255,255,0};GatewayArpProbe arp(selected);if(arp_sent)data(SecureFramePhase::arp_request,arp.issue_request());
#endif
  data(SecureFramePhase::release,dhcp_release_msdu(1,selected));send(SecureFramePhase::deauthenticate,secure_management_packet(SecureFramePhase::deauthenticate));
 }
 std::cout<<"Zero-to-three M1 responses, optional DISCOVER retry, unique sequences and no duplicate submissions passed\n";
}
