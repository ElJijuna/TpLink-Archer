#include "archer/wpa2_handshake.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
std::vector<std::uint8_t> from_hex(const std::string& s){assert(s.size()%2==0);std::vector<std::uint8_t> b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::stoul(s.substr(i,2),nullptr,16));return b;}
std::vector<std::uint8_t> file_bytes(const char* path){std::ifstream f(path,std::ios::binary);std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(f)),{});assert(!b.empty());return b;}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int main(){
 auto known=derive_wpa2_pmk("password","IEEE");auto expected=from_hex("f42c6fc52df0ebef9ebb4b90b38a5f902e83fe1b135a70e23aed762e9710a12e");assert(std::equal(expected.begin(),expected.end(),known.bytes.begin()));
 rejects([]{derive_wpa2_pmk("short","IEEE");});rejects([]{derive_wpa2_pmk("password","");});
 const auto kek=from_hex("000102030405060708090a0b0c0d0e0f"),wrapped=from_hex("1fa68b0a8112b447aef34bd8fb5a7b829d3e862371d2cfe5"),plain=from_hex("00112233445566778899aabbccddeeff");
 auto unwrapped=unwrap_wpa2_key_data(kek.data(),wrapped.data(),wrapped.size());assert(unwrapped.bytes==plain);
 auto corrupt=wrapped;corrupt.back()^=1;rejects([&]{unwrap_wpa2_key_data(kek.data(),corrupt.data(),corrupt.size());});
 const auto m1=file_bytes("tests/fixtures/special-eapol-m1-stage104.bin");const auto decoded=decode_target_eapol_m1(m1.data(),m1.size());assert(decoded);
 std::array<std::uint8_t,32> snonce{};for(unsigned i=0;i<32;++i)snonce[i]=i+1;
 auto pmk=derive_wpa2_pmk("password","Especial");auto ptk=derive_wpa2_ptk(pmk,decoded->nonce,snonce);
 const auto expected_ptk=file_bytes("tests/fixtures/wpa2-test-ptk.bin");assert(std::equal(expected_ptk.begin(),expected_ptk.end(),ptk.bytes.begin()));
 Wpa2PskHandshake handshake(derive_wpa2_pmk("password","Especial"),snonce);
 rejects([&]{handshake.issue_m2();});rejects([&]{handshake.issue_m4();});rejects([&]{handshake.pairwise_key();});
 handshake.accept_m1(m1.data(),m1.size());rejects([&]{handshake.accept_m1(m1.data(),m1.size());});
 const auto m2=handshake.issue_m2();assert(m2==file_bytes("tests/fixtures/wpa2-test-m2.bin"));assert(eapol_be16(m2.data()+7)==0);
 rejects([&]{handshake.issue_m2();});
 const auto m3=file_bytes("tests/fixtures/wpa2-test-m3.bin");
 for(unsigned at:{0u,1u,2u,4u,5u,7u,9u,17u,81u,97u,110u}){auto changed=m3;changed[at]^=1;rejects([&]{handshake.accept_m3(changed);});}
 for(std::size_t n=0;n<m3.size();++n){auto changed=m3;changed.resize(n);rejects([&]{handshake.accept_m3(changed);});}
 auto old=m3;std::copy(decoded->replay.begin(),decoded->replay.end(),old.begin()+9);sign_wpa2_eapol(old,ptk);rejects([&]{handshake.accept_m3(old);});
 rejects([&]{handshake.accept_m3(file_bytes("tests/fixtures/wpa2-test-m3-invalid-kde.bin"));});
 handshake.accept_m3(m3);rejects([&]{handshake.accept_m3(m3);});
 const auto m4=handshake.issue_m4();assert(m4==file_bytes("tests/fixtures/wpa2-test-m4.bin"));assert(handshake.m4_issued()&&handshake.group_key_id()==1);
 assert(std::equal(ptk.bytes.begin()+32,ptk.bytes.end(),handshake.pairwise_key()));
 for(unsigned i=0;i<16;++i)assert(handshake.group_key()[i]==0xa0+i);
 rejects([&]{handshake.issue_m4();});
 // A new M1 replay counter with the same ANonce needs a matching M2 MIC.
 Wpa2PskHandshake retrying(derive_wpa2_pmk("password","Especial"),snonce);
 rejects([&]{retrying.issue_m2_for_m1_retry(m1.data(),m1.size());});
 retrying.accept_m1(m1.data(),m1.size());retrying.issue_m2();
 auto retried_m1=m1;
 const auto update_m1=[&](unsigned replay,bool change_nonce=false){
  retried_m1=m1;const unsigned header=(retried_m1[0]&0x80)?26:24;
  retried_m1[header+8+9+7]=replay;if(change_nonce)retried_m1[header+8+17]^=1;
  channel_put32(retried_m1.data()+retried_m1.size()-4,ieee_fcs(retried_m1.data(),retried_m1.size()-4));
 };
 rejects([&]{retrying.issue_m2_for_m1_retry(m1.data(),m1.size());});
 update_m1(2,true);rejects([&]{retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());});
 for(unsigned replay=2;replay<=4;++replay){
  update_m1(replay);auto response=retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());
  assert(response[16]==replay&&std::equal(snonce.begin(),snonce.end(),response.begin()+17));
  auto expected=response;sign_wpa2_eapol(expected,ptk);assert(expected==response);
  rejects([&]{retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());});
 }
 update_m1(5);rejects([&]{retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());});
 auto current_m3=m3;current_m3[16]=5;sign_wpa2_eapol(current_m3,ptk);
 retrying.accept_m3(current_m3);rejects([&]{retrying.accept_m3(current_m3);});
 rejects([&]{retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());});retrying.issue_m4();
 rejects([&]{retrying.issue_m2_for_m1_retry(retried_m1.data(),retried_m1.size());});
 auto random=fresh_wpa2_snonce();assert(std::any_of(random.begin(),random.end(),[](auto b){return b!=0;}));
 std::cout<<"WPA2 PMK/PTK/MIC independent vectors, RFC3394, M1-M4 ordering/replay/integrity and one-shot guards passed\n";
}
