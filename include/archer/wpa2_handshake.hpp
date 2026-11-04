#pragma once
#include "archer/eapol_m1.hpp"
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonKeyDerivation.h>
#include <CommonCrypto/CommonRandom.h>
#include <CommonCrypto/CommonSymmetricKeywrap.h>
#include <string_view>
namespace archer {
inline void wipe_secret(void* memory,std::size_t size)noexcept{
 auto* p=static_cast<volatile std::uint8_t*>(memory);while(size--)*p++=0;
}
template<std::size_t N>struct SecretBytes {
 std::array<std::uint8_t,N> bytes{};
 SecretBytes()=default;SecretBytes(const SecretBytes&)=delete;SecretBytes& operator=(const SecretBytes&)=delete;
 SecretBytes(SecretBytes&& other)noexcept:bytes(other.bytes){wipe_secret(other.bytes.data(),N);}
 SecretBytes& operator=(SecretBytes&& other)noexcept{if(this!=&other){wipe_secret(bytes.data(),N);bytes=other.bytes;wipe_secret(other.bytes.data(),N);}return *this;}
 ~SecretBytes(){wipe_secret(bytes.data(),N);}
};
struct SecretVector {
 std::vector<std::uint8_t> bytes;
 explicit SecretVector(std::size_t n):bytes(n){}
 SecretVector(const SecretVector&)=delete;SecretVector& operator=(const SecretVector&)=delete;
 SecretVector(SecretVector&& other)noexcept:bytes(std::move(other.bytes)){}
 ~SecretVector(){wipe_secret(bytes.data(),bytes.size());}
};
inline bool constant_time_equal(const std::uint8_t* a,const std::uint8_t* b,std::size_t n){
 unsigned difference=0;for(std::size_t i=0;i<n;++i)difference|=unsigned(a[i]^b[i]);return difference==0;
}
inline SecretBytes<32> derive_wpa2_pmk(std::string_view passphrase,std::string_view ssid){
 if(passphrase.size()<8||passphrase.size()>63||passphrase.find('\0')!=std::string_view::npos||ssid.empty()||ssid.size()>32)
  throw std::invalid_argument("Unsupported WPA2 passphrase/SSID extent");
 SecretBytes<32> result;
 if(CCKeyDerivationPBKDF(kCCPBKDF2,passphrase.data(),passphrase.size(),reinterpret_cast<const std::uint8_t*>(ssid.data()),ssid.size(),kCCPRFHmacAlgSHA1,4096,result.bytes.data(),result.bytes.size()))
  throw std::runtime_error("WPA2 PMK derivation failed");
 return result;
}
inline SecretBytes<48> derive_wpa2_ptk(const SecretBytes<32>& pmk,const std::array<std::uint8_t,32>& anonce,const std::array<std::uint8_t,32>& snonce){
 if(std::all_of(snonce.begin(),snonce.end(),[](auto b){return b==0;})||snonce==anonce)
  throw std::invalid_argument("Fresh distinct SNonce required");
 std::vector<std::uint8_t> input;const char label[]="Pairwise key expansion";
 input.insert(input.end(),label,label+sizeof(label));
 const auto& mac0=std::min(station_test_mac,station_target_ap);const auto& mac1=std::max(station_test_mac,station_target_ap);
 input.insert(input.end(),mac0.begin(),mac0.end());input.insert(input.end(),mac1.begin(),mac1.end());
 const auto& n0=std::min(anonce,snonce);const auto& n1=std::max(anonce,snonce);
 input.insert(input.end(),n0.begin(),n0.end());input.insert(input.end(),n1.begin(),n1.end());input.push_back(0);
 SecretBytes<48> result;
 for(unsigned block=0,at=0;at<48;++block){
  input.back()=block;SecretBytes<20> hash;
  CCHmac(kCCHmacAlgSHA1,pmk.bytes.data(),32,input.data(),input.size(),hash.bytes.data());
  const auto size=std::min(20u,48-at);std::copy_n(hash.bytes.begin(),size,result.bytes.begin()+at);at+=size;
 }
 return result;
}
inline std::array<std::uint8_t,32> fresh_wpa2_snonce(){
 std::array<std::uint8_t,32> result{};
 if(CCRandomGenerateBytes(result.data(),result.size())!=kCCSuccess||std::all_of(result.begin(),result.end(),[](auto b){return b==0;}))
  throw std::runtime_error("SNonce generation failed");
 return result;
}
inline const std::array<std::uint8_t,22> station_rsn_ie{48,20,1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,2,0,0};
inline void eapol_put_be16(std::uint8_t* p,unsigned v){p[0]=v>>8;p[1]=v;}
inline std::vector<std::uint8_t> encode_wpa2_key(unsigned info,const std::array<std::uint8_t,8>& replay,const std::array<std::uint8_t,32>& nonce,const std::vector<std::uint8_t>& data={}){
 if(data.size()>1024)throw std::invalid_argument("EAPOL key data too large");
 std::vector<std::uint8_t> result(99+data.size());result[0]=2;result[1]=3;eapol_put_be16(result.data()+2,result.size()-4);
 auto* key=result.data()+4;key[0]=2;eapol_put_be16(key+1,info); // RSN M2/M4 key_length remains0.
 std::copy(replay.begin(),replay.end(),key+5);std::copy(nonce.begin(),nonce.end(),key+13);
 eapol_put_be16(key+93,data.size());std::copy(data.begin(),data.end(),key+95);return result;
}
inline void sign_wpa2_eapol(std::vector<std::uint8_t>& eapol,const SecretBytes<48>& ptk){
 if(eapol.size()<99||eapol_be16(eapol.data()+2)!=eapol.size()-4)throw std::invalid_argument("Invalid EAPOL MIC extent");
 std::fill_n(eapol.begin()+81,16,0);SecretBytes<20> mic;
 CCHmac(kCCHmacAlgSHA1,ptk.bytes.data(),16,eapol.data(),eapol.size(),mic.bytes.data());
 std::copy_n(mic.bytes.begin(),16,eapol.begin()+81);
}
inline SecretVector unwrap_wpa2_key_data(const std::uint8_t* kek,const std::uint8_t* data,std::size_t size){
 if(size<24||size>1032||size%8)throw std::invalid_argument("Invalid AES-wrapped key extent");
 SecretVector result(size-8);std::size_t length=result.bytes.size();
 if(CCSymmetricKeyUnwrap(kCCWRAPAES,CCrfc3394_iv,CCrfc3394_ivLen,kek,16,data,size,result.bytes.data(),&length)||length!=size-8)
  throw std::runtime_error("WPA2 key unwrap integrity failed");
 return result;
}
class Wpa2PskHandshake {
 enum class Phase {ready,m1,m2,m3,m4};Phase phase_=Phase::ready;
 SecretBytes<32> pmk_;SecretBytes<48> ptk_;SecretBytes<16> gtk_;
 std::array<std::uint8_t,32> snonce_;EapolM1 m1_{};std::array<std::uint8_t,8> replay3_{};
 unsigned gtk_id_=0;std::uint64_t gtk_initial_pn_=0;
public:
 Wpa2PskHandshake(SecretBytes<32>&& pmk,const std::array<std::uint8_t,32>& snonce):pmk_(std::move(pmk)),snonce_(snonce){}
 void accept_m1(const std::uint8_t* frame,std::size_t size){
  if(phase_!=Phase::ready)throw std::logic_error("M1 already consumed");
  const auto m=decode_target_eapol_m1(frame,size);if(!m)throw std::invalid_argument("Verified target M1 required");
  m1_=*m;ptk_=derive_wpa2_ptk(pmk_,m1_.nonce,snonce_);wipe_secret(pmk_.bytes.data(),32);phase_=Phase::m1;
 }
 std::vector<std::uint8_t> issue_m2(){
  if(phase_!=Phase::m1)throw std::logic_error("M2 order/retry forbidden");phase_=Phase::m2;
  auto result=encode_wpa2_key(0x010a,m1_.replay,snonce_,{station_rsn_ie.begin(),station_rsn_ie.end()});sign_wpa2_eapol(result,ptk_);return result;
 }
 void accept_m3(const std::vector<std::uint8_t>& eapol){
  if(phase_!=Phase::m2)throw std::logic_error("M3 order/reinstallation forbidden");
  if(eapol.size()<123||eapol.size()>1131||eapol[0]!=2||eapol[1]!=3||eapol_be16(eapol.data()+2)!=eapol.size()-4)
   throw std::invalid_argument("Invalid RSN M3 envelope");
  const auto* key=eapol.data()+4;std::array<std::uint8_t,8> replay{};std::copy_n(key+5,8,replay.begin());
  if(key[0]!=2||eapol_be16(key+1)!=0x13ca||eapol_be16(key+3)!=16||replay<=m1_.replay||
     !std::equal(m1_.nonce.begin(),m1_.nonce.end(),key+13)||eapol_be16(key+93)!=eapol.size()-99)
   throw std::invalid_argument("M3 flags/replay/nonce/extent mismatch");
  auto signed_copy=eapol;sign_wpa2_eapol(signed_copy,ptk_);
  if(!constant_time_equal(signed_copy.data()+81,eapol.data()+81,16))throw std::runtime_error("M3 MIC verification failed");
  auto plain=unwrap_wpa2_key_data(ptk_.bytes.data()+16,key+95,eapol.size()-99);
  bool found_rsn=false,found_gtk=false;SecretBytes<16> gtk;unsigned id=0;
  for(std::size_t at=0;at<plain.bytes.size();){
   const auto remaining=plain.bytes.size()-at;const auto* ie=plain.bytes.data()+at;
   if(ie[0]==0xdd&&(remaining==1||ie[1]==0)){
    if(!std::all_of(ie+1,ie+remaining,[](auto b){return b==0;}))throw std::invalid_argument("Invalid key-data padding");break;
   }
   if(remaining<2||ie[1]>remaining-2)throw std::invalid_argument("Invalid unwrapped key-data IE");
   const unsigned length=ie[1];
   if(ie[0]==48){
    if(found_rsn||length!=20||!std::equal(station_rsn_ie.begin()+2,station_rsn_ie.begin()+20,ie+2)||(management_u16(ie+20)&0xc0))
     throw std::invalid_argument("M3 RSN profile changed");
    found_rsn=true;
   }else if(ie[0]==221){
    const std::array<std::uint8_t,4> gtk_type{0,0x0f,0xac,1};
    if(found_gtk||length!=22||!std::equal(gtk_type.begin(),gtk_type.end(),ie+2)||(ie[6]&0xf8)||ie[7])
     throw std::invalid_argument("Unsupported/duplicate GTK KDE");
    id=ie[6]&3;std::copy_n(ie+8,16,gtk.bytes.begin());found_gtk=true;
   }else throw std::invalid_argument("Unsupported M3 key-data IE");
   at+=2+length;
  }
  if(!found_rsn||!found_gtk)throw std::invalid_argument("M3 missing RSN/GTK");
  if(key[67]||key[68])throw std::invalid_argument("Unsupported GTK receive sequence extent");
  std::uint64_t initial_pn=0;for(unsigned i=0;i<6;++i)initial_pn|=std::uint64_t(key[61+i])<<(8*i);
  gtk_initial_pn_=initial_pn;gtk_=std::move(gtk);gtk_id_=id;replay3_=replay;phase_=Phase::m3;
 }
 std::vector<std::uint8_t> issue_m4(){
  if(phase_!=Phase::m3)throw std::logic_error("M4 order/retry forbidden");phase_=Phase::m4;
  auto result=encode_wpa2_key(0x030a,replay3_,{});sign_wpa2_eapol(result,ptk_);return result;
 }
 bool m4_issued()const{return phase_==Phase::m4;}
 std::uint64_t group_initial_pn()const{if(phase_!=Phase::m4)throw std::logic_error("Handshake incomplete");return gtk_initial_pn_;}
 unsigned group_key_id()const{if(phase_!=Phase::m4)throw std::logic_error("Handshake incomplete");return gtk_id_;}
 const std::uint8_t* pairwise_key()const{if(phase_!=Phase::m4)throw std::logic_error("Handshake incomplete");return ptk_.bytes.data()+32;}
 const std::uint8_t* group_key()const{if(phase_!=Phase::m4)throw std::logic_error("Handshake incomplete");return gtk_.bytes.data();}
};
}
