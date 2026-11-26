#include "archer/unicast_arp.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int main(){
 DhcpOffer lease;lease.address={192,0,2,10};lease.server={192,0,2,1};lease.router={192,0,2,1};lease.subnet={255,255,255,0};lease.lease_seconds=3600;
 GatewayArpProbe probe(lease);auto request=probe.issue_request();assert(request.size()==36&&request[15]==1);
 rejects([&]{probe.issue_request();});
 const std::array<std::uint8_t,6> peer{2,1,2,3,4,5};auto reply=request;reply[15]=2;
 std::copy(peer.begin(),peer.end(),reply.begin()+16);std::copy(lease.router.begin(),lease.router.end(),reply.begin()+22);
 std::copy(station_test_mac.begin(),station_test_mac.end(),reply.begin()+26);std::copy(lease.address.begin(),lease.address.end(),reply.begin()+32);
 assert(!probe.accept_authenticated_reply(reply,false,peer));
 for(unsigned at:{0u,8u,10u,12u,13u,15u,16u,22u,26u,32u}){auto bad=reply;bad[at]^=1;assert(!probe.accept_authenticated_reply(bad,true,peer));}
 for(unsigned n=0;n<36;++n){auto bad=reply;bad.resize(n);assert(!probe.accept_authenticated_reply(bad,true,peer));}
 auto padded=reply;padded.resize(54);padded.back()=1;assert(!probe.accept_authenticated_reply(padded,true,peer));padded.back()=0;
 auto wrong=peer;wrong[1]^=1;assert(!probe.accept_authenticated_reply(reply,true,wrong));
 assert(probe.accept_authenticated_reply(padded,true,peer));assert(probe.matched());assert(!probe.accept_authenticated_reply(reply,true,peer));
 GatewayArpProbe not_issued(lease);assert(!not_issued.accept_authenticated_reply(reply,true,peer));
 for(unsigned variant=0;variant<5;++variant){auto bad=lease;if(variant==0)bad.lease_seconds=0;if(variant==1)bad.router={192,0,3,1};if(variant==2)bad.subnet={255,0,255,0};if(variant==3)bad.router=bad.address;if(variant==4)bad.router={192,0,2,255};rejects([&]{GatewayArpProbe invalid(bad);});}
 // Exercise the same authenticated pairwise/replay boundary as the receiver.
 std::array<std::uint8_t,32> nonce{};for(unsigned i=0;i<32;++i)nonce[i]=i+1;
 const auto load=[](const char* name){std::ifstream f(name,std::ios::binary);assert(f);return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)),{});};
 auto m1=load("tests/fixtures/special-eapol-m1-stage104.bin"),m3=load("tests/fixtures/wpa2-test-m3.bin");
 Wpa2PskHandshake hs(derive_wpa2_pmk("password","Especial"),nonce);hs.accept_m1(m1.data(),m1.size());hs.issue_m2();hs.accept_m3(m3);hs.issue_m4();
 CcmpSession ccmp(hs);GatewayArpProbe encrypted_probe(lease);encrypted_probe.issue_request();
 std::vector<std::uint8_t> frame(24);frame[0]=8;frame[1]=0x42;
 std::copy(station_test_mac.begin(),station_test_mac.end(),frame.begin()+4);std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+10);std::copy(peer.begin(),peer.end(),frame.begin()+16);
 const auto params=ccmp_parameters(frame.data(),frame.size(),1);auto cipher=ccm8_encrypt(hs.pairwise_key(),params.nonce,params.aad,reply);
 frame.insert(frame.end(),{1,0,0,0x20,0,0,0,0});frame.insert(frame.end(),cipher.begin(),cipher.end());
 const auto with_fcs=[](std::vector<std::uint8_t>& b){auto n=b.size();b.resize(n+4);channel_put32(b.data()+n,ieee_fcs(b.data(),n));};with_fcs(frame);
 auto tampered=frame;tampered[35]^=1;channel_put32(tampered.data()+tampered.size()-4,ieee_fcs(tampered.data(),tampered.size()-4));rejects([&]{ccmp.decrypt_from_ap(tampered.data(),tampered.size());});
 auto plain=ccmp.decrypt_from_ap(frame.data(),frame.size());assert(encrypted_probe.accept_authenticated_reply(plain.bytes,true,peer));
 rejects([&]{ccmp.decrypt_from_ap(frame.data(),frame.size());});
 std::cout<<"One leased on-link ARP request, strict pairwise reply identity and no retries passed\n";
}
