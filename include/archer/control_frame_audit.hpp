#pragma once
#include "archer/passive_scan.hpp"
#include <set>
namespace archer {
// ACK/CTS have a receiver address only; never infer a transmitter/BSSID.
class ControlFrameAudit {
 unsigned ack_=0,cts_=0,to_test_=0,bad_fcs_=0;
 std::set<std::array<std::uint8_t,6>> receivers_;
public:
 bool observe(const std::vector<std::uint8_t>& bytes,const BeRxMetadata& m){
  const auto at=m.packet.payload_offset,n=m.packet.payload_bytes;
  if(!m.normal_80211_candidate()||m.fcs_policy!=RxFcsPolicy::appended||at>bytes.size()||n>bytes.size()-at||n!=14)return false;
  const auto* p=bytes.data()+at;const unsigned fc=p[0]|(unsigned(p[1])<<8);
  if((p[0]!=0xd4&&p[0]!=0xc4)||(fc&0xc700))return false;
  if(ieee_fcs(p,10)!=channel_u32(p+10)){++bad_fcs_;return false;}
  std::array<std::uint8_t,6> receiver{};std::copy(p+4,p+10,receiver.begin());
  receivers_.insert(receiver);if(p[0]==0xd4)++ack_;else ++cts_;
  if(receiver==std::array<std::uint8_t,6>{2,0,0,0x89,0x22,0xa1})++to_test_;
  return true;
 }
 unsigned ack()const{return ack_;}unsigned cts()const{return cts_;}
 unsigned bad_fcs()const{return bad_fcs_;}unsigned to_test()const{return to_test_;}
 void write_json(std::ostream& out)const{
  out<<"{\"ack_frames\":"<<ack_<<",\"cts_frames\":"<<cts_<<",\"control_frames_to_test_mac\":"<<to_test_
     <<",\"bad_control_fcs\":"<<bad_fcs_<<",\"distinct_receiver_addresses\":"<<receivers_.size()
     <<",\"transmitter_identity_known\":false,\"attributed_to_our_tx\":false}\n";
 }
};
}
