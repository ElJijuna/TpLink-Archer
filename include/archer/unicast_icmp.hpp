#pragma once
#include "archer/unicast_arp.hpp"
namespace archer {
// One RFC792 Echo to the leased on-link gateway. No sockets, routes or retries.
class GatewayIcmpProbe {
 DhcpOffer lease_;std::array<std::uint8_t,6> peer_;std::array<std::uint8_t,16> cookie_;unsigned id_;bool issued_=false,matched_=false;
public:
 GatewayIcmpProbe(const DhcpOffer& lease,const std::array<std::uint8_t,6>& peer,unsigned id,const std::array<std::uint8_t,16>& cookie):lease_(lease),peer_(peer),cookie_(cookie),id_(id){
  GatewayArpProbe validate(lease);(void)validate;
  if(id>65535||(peer[0]&1)||std::all_of(peer.begin(),peer.end(),[](auto b){return b==0;})||peer==station_test_mac)throw std::invalid_argument("Valid resolved gateway and echo identifier required");
 }
 std::vector<std::uint8_t> issue_request(){
  if(issued_)throw std::logic_error("Echo already issued; no retry");issued_=true;
  std::vector<std::uint8_t> b(52);const std::array<std::uint8_t,8> snap{0xaa,0xaa,3,0,0,0,8,0};std::copy(snap.begin(),snap.end(),b.begin());
  auto* ip=b.data()+8;ip[0]=0x45;eapol_put_be16(ip+2,44);eapol_put_be16(ip+4,id_);eapol_put_be16(ip+6,0x4000);ip[8]=64;ip[9]=1;
  std::copy(lease_.address.begin(),lease_.address.end(),ip+12);std::copy(lease_.router.begin(),lease_.router.end(),ip+16);eapol_put_be16(ip+10,internet_checksum(ip,20));
  auto* icmp=ip+20;icmp[0]=8;eapol_put_be16(icmp+4,id_);eapol_put_be16(icmp+6,1);std::copy(cookie_.begin(),cookie_.end(),icmp+8);eapol_put_be16(icmp+2,internet_checksum(icmp,24));return b;
 }
 bool accept_authenticated_reply(const std::vector<std::uint8_t>& b,bool pairwise,const std::array<std::uint8_t,6>& source){
  const std::array<std::uint8_t,8> snap{0xaa,0xaa,3,0,0,0,8,0};
  if(!issued_||matched_||!pairwise||source!=peer_||b.size()<52||b.size()>110||!std::equal(snap.begin(),snap.end(),b.begin()))return false;
  const auto* ip=b.data()+8;const unsigned ihl=(ip[0]&15)*4,total=eapol_be16(ip+2);
  if(ip[0]>>4!=4||ihl<20||ihl>60||total!=ihl+24||b.size()<8+total||b.size()>8+total+18||!ip[8]||ip[9]!=1||eapol_be16(ip+6)&0xbfff||internet_checksum(ip,ihl)||
     !std::equal(lease_.router.begin(),lease_.router.end(),ip+12)||!std::equal(lease_.address.begin(),lease_.address.end(),ip+16)||!std::all_of(b.begin()+8+total,b.end(),[](auto v){return v==0;}))return false;
  const auto* icmp=ip+ihl;
  if(icmp[0]||icmp[1]||internet_checksum(icmp,24)||eapol_be16(icmp+4)!=id_||eapol_be16(icmp+6)!=1||!std::equal(cookie_.begin(),cookie_.end(),icmp+8))return false;
  matched_=true;return true;
 }
 bool matched()const{return matched_;}
};
}
