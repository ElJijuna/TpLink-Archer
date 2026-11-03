#include "archer/control_frame_audit.hpp"
#include <cassert>
std::vector<std::uint8_t> packet(unsigned fc){
 std::vector<std::uint8_t>b{static_cast<std::uint8_t>(fc),0,0,0,2,0,0,0x89,0x22,0xa1};
 const auto fcs=archer::ieee_fcs(b.data(),b.size());for(unsigned i=0;i<4;++i)b.push_back(fcs>>(8*i));return b;
}
archer::BeRxMetadata meta(const std::vector<std::uint8_t>& b){archer::BeRxMetadata m;m.packet.payload_bytes=b.size();m.header_converted=false;m.fcs_policy=archer::RxFcsPolicy::appended;return m;}
int main(){
 archer::ControlFrameAudit a;auto ack=packet(0xd4),cts=packet(0xc4);
 assert(a.observe(ack,meta(ack))&&a.ack()==1&&a.to_test()==1);
 assert(a.observe(cts,meta(cts))&&a.cts()==1&&a.to_test()==2);
 ack.back()^=1;assert(!a.observe(ack,meta(ack))&&a.bad_fcs()==1&&a.ack()==1);
 auto invalid=meta(cts);invalid.packet.payload_offset=cts.size()+1;assert(!a.observe(cts,invalid));
 invalid=meta(cts);invalid.fcs_policy=archer::RxFcsPolicy::unknown;assert(!a.observe(cts,invalid));
 auto unrelated=packet(0xb4);assert(!a.observe(unrelated,meta(unrelated)));
}
