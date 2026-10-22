#include "archer/interface_role.hpp"
#include "archer/command_tracker.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::invalid_argument&){yes=true;}assert(yes);}
std::vector<std::uint8_t> ack(unsigned function,unsigned seq,unsigned status=0){
 std::vector<std::uint8_t>b(12);archer::channel_put32(b.data(),0x101);archer::channel_put32(b.data()+4,12);
 archer::channel_put32(b.data()+8,0x21|(function<<8)|(status<<16)|(seq<<24));return b;
}
int main(){
 using namespace archer;
 const std::vector<std::uint8_t> golden{0x21,0,0,9,20,0x80,0,0,0,1,0,4,0,0x10,0x2c,0,0,0,0,0};
 assert(encode_unlinked_station_join(9,0,0,0,StationCapability::legacy)==golden);
 auto upper=encode_unlinked_station_join(255,255,4,3,StationCapability::eht);
 assert(channel_u32(upper.data()+8)==0x04800dff&&channel_u32(upper.data()+12)==0x2c1ff2&&channel_u32(upper.data()+16)==0);
 rejects([]{encode_unlinked_station_join(256,0,0,0,StationCapability::legacy);});
 rejects([]{encode_unlinked_station_join(0,256,0,0,StationCapability::legacy);});
 rejects([]{encode_unlinked_station_join(0,0,5,0,StationCapability::legacy);});
 rejects([]{encode_unlinked_station_join(0,0,0,4,StationCapability::legacy);});
 rejects([]{encode_unlinked_station_join(0,0,0,0,static_cast<StationCapability>(3));});
 CommandTracker join(1,8,0,9);auto wrong=ack(4,9);assert(!join.observe(wrong.data(),wrong.size())&&!join.completion);
 wrong=ack(0,8);assert(!join.observe(wrong.data(),wrong.size())&&!join.completion);
 auto ok=ack(0,9);assert(join.observe(ok.data(),ok.size())&&join.completion==0);
 assert(join.observe(ok.data(),ok.size()));auto conflict=ack(0,9,1);rejects([&]{join.observe(conflict.data(),conflict.size());});
 CommandTracker error(1,8,0,10);auto failed=ack(0,10,7);assert(error.observe(failed.data(),failed.size())&&error.completion==7);
 ok.pop_back();rejects([&]{join.observe(ok.data(),ok.size());});
 std::cout<<"BE JOIN_INFO golden frames and ACK isolation passed\n";
}
