#pragma once
#include "archer/secure_network.hpp"
namespace archer {
inline bool dhcp_unicast_ipv4(const std::array<std::uint8_t,4>& address){
 return address[0]&&address[0]!=127&&address[0]<224;
}
inline std::vector<std::uint8_t> dhcp_selecting_request(std::uint32_t xid,const DhcpOffer& offer,unsigned elapsed_seconds=0){
 if(!dhcp_unicast_ipv4(offer.address)||!dhcp_unicast_ipv4(offer.server))throw std::invalid_argument("Invalid selected DHCP offer");
 auto msdu=dhcp_discover_msdu(xid,true,elapsed_seconds);
 std::fill(msdu.begin()+276,msdu.end(),0);
 std::vector<std::uint8_t> options{53,1,3,50,4};
 options.insert(options.end(),offer.address.begin(),offer.address.end());options.insert(options.end(),{54,4});
 options.insert(options.end(),offer.server.begin(),offer.server.end());options.insert(options.end(),{61,7,1});
 options.insert(options.end(),station_test_mac.begin(),station_test_mac.end());options.insert(options.end(),{55,3,1,3,6,255});
 std::copy(options.begin(),options.end(),msdu.begin()+276);return msdu;
}
inline std::vector<std::uint8_t> dhcp_release_msdu(std::uint32_t xid,const DhcpOffer& lease){
 if(!dhcp_unicast_ipv4(lease.address)||!dhcp_unicast_ipv4(lease.server)||!lease.lease_seconds)throw std::invalid_argument("Acknowledged DHCP lease required");
 auto msdu=dhcp_discover_msdu(xid,false);auto* ip=msdu.data()+8;auto* boot=msdu.data()+36;
 std::copy(lease.address.begin(),lease.address.end(),boot+12);
 std::copy(lease.address.begin(),lease.address.end(),ip+12);std::copy(lease.server.begin(),lease.server.end(),ip+16);
 ip[10]=ip[11]=0;eapol_put_be16(ip+10,internet_checksum(ip,20));
 std::fill(msdu.begin()+276,msdu.end(),0);std::vector<std::uint8_t> options{53,1,7,54,4};
 options.insert(options.end(),lease.server.begin(),lease.server.end());options.insert(options.end(),{61,7,1});
 options.insert(options.end(),station_test_mac.begin(),station_test_mac.end());options.push_back(255);
 std::copy(options.begin(),options.end(),msdu.begin()+276);return msdu;
}
// Protocol only. The caller must authenticate CCMP before providing an MSDU.
// No address configuration, submission, retransmission or delivery claim here.
class DhcpSelectingExchange {
 enum class State {selecting,offered,request_issued,acknowledged,release_issued};
 State state_=State::selecting;std::uint32_t xid_;DhcpOffer selected_{};
public:
 explicit DhcpSelectingExchange(std::uint32_t xid):xid_(xid){if(!xid)throw std::invalid_argument("Nonzero DHCP transaction required");}
 bool accept_authenticated_offer(const std::vector<std::uint8_t>& msdu){
  if(state_!=State::selecting)return false;auto offer=decode_dhcp_offer(msdu,xid_);if(!offer)return false;
  selected_=*offer;state_=State::offered;return true;
 }
 std::vector<std::uint8_t> issue_request(unsigned elapsed_seconds=0){
  if(state_!=State::offered)throw std::logic_error("DHCP REQUEST order/retry forbidden");
  auto request=dhcp_selecting_request(xid_,selected_,elapsed_seconds);state_=State::request_issued;return request;
 }
 bool accept_authenticated_ack(const std::vector<std::uint8_t>& msdu){
  if(state_!=State::request_issued)return false;auto ack=decode_dhcp_reply(msdu,xid_,5);
  if(!ack||ack->address!=selected_.address||ack->server!=selected_.server)return false;
  selected_=*ack;state_=State::acknowledged;return true;
 }
 bool lease_acknowledged()const{return state_==State::acknowledged||state_==State::release_issued;}
 std::vector<std::uint8_t> issue_release(){
  if(state_!=State::acknowledged)throw std::logic_error("DHCP RELEASE order/retry forbidden");
  auto release=dhcp_release_msdu(xid_,selected_);state_=State::release_issued;return release;
 }
};
}
