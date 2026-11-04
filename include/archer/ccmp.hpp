#pragma once
#include "archer/wpa2_handshake.hpp"
#include <CommonCrypto/CommonCryptor.h>
namespace archer {
inline SecretBytes<16> aes128_block(const std::uint8_t* key,const std::uint8_t* block){
 SecretBytes<16> output;std::size_t size=0;
 if(CCCrypt(kCCEncrypt,kCCAlgorithmAES,kCCOptionECBMode,key,16,nullptr,block,16,output.bytes.data(),16,&size)!=kCCSuccess||size!=16)
  throw std::runtime_error("AES block operation failed");
 return output;
}
inline SecretBytes<16> ccm_mac(const std::uint8_t* key,const std::array<std::uint8_t,13>& nonce,const std::vector<std::uint8_t>& aad,const std::uint8_t* plain,std::size_t length){
 if(length>65535||aad.size()>=65280)throw std::invalid_argument("Unsupported CCM extent");
 SecretBytes<16> block;block.bytes[0]=(aad.empty()?0:0x40)|0x19;
 std::copy(nonce.begin(),nonce.end(),block.bytes.begin()+1);eapol_put_be16(block.bytes.data()+14,length);
 auto state=aes128_block(key,block.bytes.data());
 const auto append=[&](const std::uint8_t* p,std::size_t n){
  for(std::size_t at=0;at<n;at+=16){
   block.bytes.fill(0);std::copy_n(p+at,std::min<std::size_t>(16,n-at),block.bytes.begin());
   for(unsigned i=0;i<16;++i)block.bytes[i]^=state.bytes[i];state=aes128_block(key,block.bytes.data());
  }
 };
 if(!aad.empty()){
  std::vector<std::uint8_t> encoded(((aad.size()+2+15)/16)*16);eapol_put_be16(encoded.data(),aad.size());
  std::copy(aad.begin(),aad.end(),encoded.begin()+2);append(encoded.data(),encoded.size());
 }
 append(plain,length);return state;
}
inline SecretBytes<16> ccm_counter(const std::uint8_t* key,const std::array<std::uint8_t,13>& nonce,unsigned counter){
 SecretBytes<16> block;block.bytes[0]=1;std::copy(nonce.begin(),nonce.end(),block.bytes.begin()+1);
 eapol_put_be16(block.bytes.data()+14,counter);return aes128_block(key,block.bytes.data());
}
inline std::vector<std::uint8_t> ccm8_encrypt(const std::uint8_t* key,const std::array<std::uint8_t,13>& nonce,const std::vector<std::uint8_t>& aad,const std::vector<std::uint8_t>& plain){
 const auto mac=ccm_mac(key,nonce,aad,plain.data(),plain.size());const auto s0=ccm_counter(key,nonce,0);
 std::vector<std::uint8_t> output(plain.size()+8);
 for(std::size_t at=0;at<plain.size();at+=16){const auto stream=ccm_counter(key,nonce,1+at/16);for(std::size_t i=0;i<std::min<std::size_t>(16,plain.size()-at);++i)output[at+i]=plain[at+i]^stream.bytes[i];}
 for(unsigned i=0;i<8;++i)output[plain.size()+i]=mac.bytes[i]^s0.bytes[i];return output;
}
inline SecretVector ccm8_decrypt(const std::uint8_t* key,const std::array<std::uint8_t,13>& nonce,const std::vector<std::uint8_t>& aad,const std::uint8_t* cipher,std::size_t size){
 if(size<8||size-8>65535||aad.size()>=65280)throw std::invalid_argument("Unsupported CCM ciphertext extent");
 SecretVector plain(size-8);
 for(std::size_t at=0;at<plain.bytes.size();at+=16){const auto stream=ccm_counter(key,nonce,1+at/16);for(std::size_t i=0;i<std::min<std::size_t>(16,plain.bytes.size()-at);++i)plain.bytes[at+i]=cipher[at+i]^stream.bytes[i];}
 const auto mac=ccm_mac(key,nonce,aad,plain.bytes.data(),plain.bytes.size());const auto s0=ccm_counter(key,nonce,0);
 SecretBytes<8> expected;for(unsigned i=0;i<8;++i)expected.bytes[i]=mac.bytes[i]^s0.bytes[i];
 if(!constant_time_equal(expected.bytes.data(),cipher+plain.bytes.size(),8))throw std::runtime_error("CCMP authentication failed");
 return plain;
}
struct CcmpParameters {unsigned header=0,tid=0,replay_slot=16;std::vector<std::uint8_t> aad;std::array<std::uint8_t,13> nonce{};};
inline CcmpParameters ccmp_parameters(const std::uint8_t* frame,std::size_t size,std::uint64_t pn){
 if(size<24||!pn||pn>=0x1000000000000ULL)throw std::invalid_argument("Invalid CCMP header/PN");
 const auto fc=management_u16(frame);
 if((fc&0x40ff)!=0x4008&&(fc&0x40ff)!=0x4088)throw std::invalid_argument("Unsupported CCMP frame type");
 if((fc&0x300)==0x300||!(fc&0x300)||fc&0x8400||management_u16(frame+22)&15)throw std::invalid_argument("CCMP WDS/order/fragment unsupported");
 CcmpParameters result;const bool qos=fc&0x80;result.header=qos?26:24;
 if(size<result.header||(qos&&(frame[24]&0x80)))throw std::invalid_argument("CCMP A-MSDU/short QoS unsupported");
 result.tid=qos?(frame[24]&15):0;result.replay_slot=qos?result.tid:16;result.nonce[0]=result.tid;
 std::copy_n(frame+10,6,result.nonce.begin()+1);for(unsigned i=0;i<6;++i)result.nonce[7+i]=pn>>(8*(5-i));
 result.aad={std::uint8_t(frame[0]&0x8f),std::uint8_t(frame[1]&0xc7)};
 result.aad.insert(result.aad.end(),frame+4,frame+22);result.aad.push_back(frame[22]&15);result.aad.push_back(0);
 if(qos){result.aad.push_back(result.tid);result.aad.push_back(0);}return result;
}
class CcmpSession {
 SecretBytes<16> pairwise_,group_;unsigned group_id_;std::uint64_t tx_pn_=0;
 std::array<std::array<std::array<std::uint64_t,17>,4>,2> rx_pn_{};
public:
 explicit CcmpSession(const Wpa2PskHandshake& handshake):group_id_(handshake.group_key_id()){
  rx_pn_[1][group_id_].fill(handshake.group_initial_pn());
  std::copy_n(handshake.pairwise_key(),16,pairwise_.bytes.begin());std::copy_n(handshake.group_key(),16,group_.bytes.begin());
 }
 std::vector<std::uint8_t> encrypt_to_ap(const std::vector<std::uint8_t>& msdu,const std::array<std::uint8_t,6>& destination,unsigned sequence){
  if(sequence>4095||msdu.size()>4096||tx_pn_>=0xffffffffffffULL)throw std::invalid_argument("CCMP TX bound exhausted");
  const auto pn=++tx_pn_; // Consumed before encryption/submission, never retried.
  std::vector<std::uint8_t> frame(24);frame[0]=8;frame[1]=0x41;
  std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+4);std::copy(station_test_mac.begin(),station_test_mac.end(),frame.begin()+10);std::copy(destination.begin(),destination.end(),frame.begin()+16);
  frame[22]=sequence<<4;frame[23]=sequence>>4;
  const auto parameters=ccmp_parameters(frame.data(),frame.size(),pn);
  const std::array<std::uint8_t,8> iv{std::uint8_t(pn),std::uint8_t(pn>>8),0,0x20,std::uint8_t(pn>>16),std::uint8_t(pn>>24),std::uint8_t(pn>>32),std::uint8_t(pn>>40)};
  frame.insert(frame.end(),iv.begin(),iv.end());const auto cipher=ccm8_encrypt(pairwise_.bytes.data(),parameters.nonce,parameters.aad,msdu);frame.insert(frame.end(),cipher.begin(),cipher.end());return frame;
 }
 SecretVector decrypt_from_ap(const std::uint8_t* frame,std::size_t size){
  if(!station_fcs_valid(frame,size)||size<44)throw std::invalid_argument("CRC-valid CCMP RX required");size-=4;
  if((management_u16(frame)&0x300)!=0x200||!std::equal(station_target_ap.begin(),station_target_ap.end(),frame+10))throw std::invalid_argument("CCMP RX source/direction mismatch");
  const bool group=frame[4]&1;
  if(!group&&!std::equal(station_test_mac.begin(),station_test_mac.end(),frame+4))throw std::invalid_argument("CCMP RX receiver mismatch");
  const unsigned header=(frame[0]&0x80)?26:24;
  if(size<header+16)throw std::invalid_argument("CCMP RX too short");
  const auto* iv=frame+header;const unsigned id=iv[3]>>6;
  if(iv[2]||(iv[3]&0x3f)!=0x20||(group?id!=group_id_:id!=0))throw std::invalid_argument("CCMP key ID/ExtIV mismatch");
  const std::uint64_t pn=std::uint64_t(iv[0])|(std::uint64_t(iv[1])<<8)|(std::uint64_t(iv[4])<<16)|(std::uint64_t(iv[5])<<24)|(std::uint64_t(iv[6])<<32)|(std::uint64_t(iv[7])<<40);
  const auto parameters=ccmp_parameters(frame,size,pn);
  if(pn<=rx_pn_[group?1:0][id][parameters.replay_slot])throw std::runtime_error("CCMP replay rejected");
  auto plain=ccm8_decrypt(group?group_.bytes.data():pairwise_.bytes.data(),parameters.nonce,parameters.aad,iv+8,size-header-8);
  rx_pn_[group?1:0][id][parameters.replay_slot]=pn;return plain; // Commit PN only after authenticated decryption.
 }
};
}
