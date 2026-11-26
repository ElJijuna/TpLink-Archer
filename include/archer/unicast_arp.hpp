#pragma once
#include "archer/dhcp_selecting.hpp"
namespace archer {
// Only a leased on-link gateway, not an arbitrary host. No I/O or IP configuration.
class GatewayArpProbe {
 DhcpOffer lease_;bool issued_=false,matched_=false;
public:
 explicit GatewayArpProbe(const DhcpOffer& lease):lease_(lease){
  const auto mask=network_u32(lease.subnet.data()),host=network_u32(lease.address.data()),gateway=network_u32(lease.router.data());
  const auto inverse=~mask;
  if(!lease.lease_seconds||!dhcp_unicast_ipv4(lease.address)||!dhcp_unicast_ipv4(lease.router)||!mask||!inverse||(inverse&(inverse+1))||
     (host&mask)!=(gateway&mask)||host==gateway||!(host&inverse)||(host&inverse)==inverse||!(gateway&inverse)||(gateway&inverse)==inverse)
   throw std::invalid_argument("Leased address and on-link DHCP gateway required for ARP");
 }
 std::vector<std::uint8_t> issue_request(){
  if(issued_)throw std::logic_error("ARP request already issued; no retry");issued_=true;
  std::vector<std::uint8_t> b{0xaa,0xaa,3,0,0,0,8,6,0,1,8,0,6,4,0,1};
  b.insert(b.end(),station_test_mac.begin(),station_test_mac.end());b.insert(b.end(),lease_.address.begin(),lease_.address.end());
  b.insert(b.end(),6,0);b.insert(b.end(),lease_.router.begin(),lease_.router.end());return b;
 }
 bool accept_authenticated_reply(const std::vector<std::uint8_t>& b,bool pairwise,const std::array<std::uint8_t,6>& ethernet_source){
  if(!issued_||matched_||!pairwise||b.size()<36||b.size()>54)return false;
  const std::array<std::uint8_t,16> prefix{0xaa,0xaa,3,0,0,0,8,6,0,1,8,0,6,4,0,2};
  if(!std::equal(prefix.begin(),prefix.end(),b.begin())||!std::all_of(b.begin()+36,b.end(),[](auto v){return v==0;})||
     (b[16]&1)||std::all_of(b.begin()+16,b.begin()+22,[](auto v){return v==0;})||
     std::equal(station_test_mac.begin(),station_test_mac.end(),b.begin()+16)||
     !std::equal(ethernet_source.begin(),ethernet_source.end(),b.begin()+16)||
     !std::equal(lease_.router.begin(),lease_.router.end(),b.begin()+22)||
     !std::equal(station_test_mac.begin(),station_test_mac.end(),b.begin()+26)||
     !std::equal(lease_.address.begin(),lease_.address.end(),b.begin()+32))return false;
  matched_=true;return true;
 }
 bool matched()const{return matched_;}
};
}
