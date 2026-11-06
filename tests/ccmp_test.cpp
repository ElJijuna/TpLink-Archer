#include "archer/ccmp.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
std::vector<std::uint8_t> hex(const std::string& s){std::vector<std::uint8_t> b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::stoul(s.substr(i,2),nullptr,16));return b;}
std::vector<std::uint8_t> read(const char* name){std::ifstream f(name,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
void fcs(std::vector<std::uint8_t>& f){auto n=f.size();f.resize(n+4);channel_put32(f.data()+n,ieee_fcs(f.data(),n));}
int main(){
 const auto key=hex("c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"),nv=hex("00000003020100a0a1a2a3a4a5");std::array<std::uint8_t,13> nonce{};std::copy(nv.begin(),nv.end(),nonce.begin());
 const auto aad=hex("0001020304050607"),plain=hex("08090a0b0c0d0e0f101112131415161718191a1b1c1d1e");
 const auto cipher=ccm8_encrypt(key.data(),nonce,aad,plain);assert(cipher==hex("588c979a61c663d2f066d0c2c0f989806d5f6b61dac38417e8d12cfdf926e0"));
 assert(ccm8_decrypt(key.data(),nonce,aad,cipher.data(),cipher.size()).bytes==plain);
 for(std::size_t i=0;i<cipher.size();++i){auto changed=cipher;changed[i]^=1;rejects([&]{ccm8_decrypt(key.data(),nonce,aad,changed.data(),changed.size());});}
 for(unsigned n:{0u,1u,15u,16u,17u,255u,256u,4096u}){std::vector<std::uint8_t> input(n,0x5a);auto c=ccm8_encrypt(key.data(),nonce,{},input);assert(ccm8_decrypt(key.data(),nonce,{},c.data(),c.size()).bytes==input);}
 std::array<std::uint8_t,32> snonce{};for(unsigned i=0;i<32;++i)snonce[i]=i+1;
 Wpa2PskHandshake hs(derive_wpa2_pmk("password","Especial"),snonce);auto m1=read("tests/fixtures/special-eapol-m1-stage104.bin");hs.accept_m1(m1.data(),m1.size());hs.issue_m2();hs.accept_m3(read("tests/fixtures/wpa2-test-m3.bin"));hs.issue_m4();
 CcmpSession session(hs);auto outgoing=session.encrypt_to_ap(plain,station_target_ap,16);auto next=session.encrypt_to_ap(plain,station_target_ap,17);assert(outgoing[24]==1&&next[24]==2);assert(outgoing==read("tests/fixtures/ccmp-openssl-test-frame.bin"));
 std::vector<std::uint8_t> incoming(24);incoming[0]=8;incoming[1]=0x42;std::copy(station_test_mac.begin(),station_test_mac.end(),incoming.begin()+4);std::copy(station_target_ap.begin(),station_target_ap.end(),incoming.begin()+10);std::copy(station_target_ap.begin(),station_target_ap.end(),incoming.begin()+16);
 auto params=ccmp_parameters(incoming.data(),incoming.size(),1);auto encrypted=ccm8_encrypt(hs.pairwise_key(),params.nonce,params.aad,plain);
 incoming.insert(incoming.end(),{1,0,0,0x20,0,0,0,0});incoming.insert(incoming.end(),encrypted.begin(),encrypted.end());fcs(incoming);
 auto bad=incoming;bad[35]^=1;bad.resize(bad.size()-4);fcs(bad);rejects([&]{session.decrypt_from_ap(bad.data(),bad.size());});
 assert(session.decrypt_from_ap(incoming.data(),incoming.size()).bytes==plain);rejects([&]{session.decrypt_from_ap(incoming.data(),incoming.size());});
 // M3 supplies the initial GTK replay floor; authenticated group packets below
 // it must not be accepted, even on a newly constructed session.
 Wpa2PskHandshake with_rsc(derive_wpa2_pmk("password","Especial"),snonce);with_rsc.accept_m1(m1.data(),m1.size());with_rsc.issue_m2();
 auto m3=read("tests/fixtures/wpa2-test-m3.bin");m3[65]=1;auto parsed=decode_target_eapol_m1(m1.data(),m1.size());auto pmk=derive_wpa2_pmk("password","Especial");auto ptk=derive_wpa2_ptk(pmk,parsed->nonce,snonce);sign_wpa2_eapol(m3,ptk);with_rsc.accept_m3(m3);with_rsc.issue_m4();CcmpSession group_session(with_rsc);
 const auto group_frame=[&](unsigned pn){std::vector<std::uint8_t> f(24);f[0]=8;f[1]=0x42;std::fill(f.begin()+4,f.begin()+10,255);std::copy(station_target_ap.begin(),station_target_ap.end(),f.begin()+10);std::copy(station_target_ap.begin(),station_target_ap.end(),f.begin()+16);auto p=ccmp_parameters(f.data(),f.size(),pn);auto c=ccm8_encrypt(with_rsc.group_key(),p.nonce,p.aad,plain);f.insert(f.end(),{std::uint8_t(pn),0,0,0x60,0,0,0,0});f.insert(f.end(),c.begin(),c.end());fcs(f);return f;};
 const auto old_group=group_frame(1),new_group=group_frame(2);rejects([&]{group_session.decrypt_from_ap(old_group.data(),old_group.size());});assert(group_session.decrypt_from_ap(new_group.data(),new_group.size()).bytes==plain);rejects([&]{group_session.decrypt_from_ap(new_group.data(),new_group.size());});
 std::cout<<"RFC3610 CCM vector, tampering, block boundaries, CCMP PN consumption and replay after authenticated RX passed\n";
}
