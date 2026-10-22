#include "archer/interface_tables.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::invalid_argument&){yes=true;}assert(yes);}
int main(){
 using namespace archer;
 const std::array<unsigned,35> cmac_words{
 0x2a001115,0x808c,0x80,4,0x400a0004,0,0,0xffff0000,0x2aa,0xb000,0xb8109,
 0,0,0,0,0,0,0,0,0xfff7ffff,0xffffffff,0xffffffff,0xffffffff,0xffffdfff,
 0xffff7fff,0x80ffffff,0xefffefff,0xffff,0,0,0,0,0,0xffffffff,0x0fffffff};
 auto cmac=encode_default_station_cmac_g7(42,0,false);assert(cmac.size()==140);
 for(unsigned i=0;i<cmac_words.size();++i)assert(channel_u32(cmac.data()+i*4)==cmac_words[i]);
 auto reports=encode_default_station_cmac_g7(42,127,true);
 assert(channel_u32(reports.data()+8)==255&&channel_u32(reports.data()+12)==0x200004);
 const std::array<unsigned,35> dmac_words{
 0x2b000c15,0x8c,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffe0,0xffffff0f,
 0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffff,0,0,0};
 auto dmac=encode_default_station_dmac_v2(43,0);assert(dmac.size()==140);
 for(unsigned i=0;i<dmac_words.size();++i)assert(channel_u32(dmac.data()+i*4)==dmac_words[i]);
 rejects([]{encode_default_station_cmac_g7(0,128,false);});rejects([]{encode_default_station_dmac_v2(0,128);});
 rejects([]{encode_default_station_dmac_v2(256,0);});
 auto pause=encode_station_macid_pause_sleep(44,127,true);
 assert(pause.size()==264&&channel_u32(pause.data())==0x2c002825&&channel_u32(pause.data()+4)==0x4108);
 for(unsigned i=0;i<64;++i)assert(channel_u32(pause.data()+8+4*i)==((i==3||i==7||i==11||i==15)?0x80000000u:0u));
 auto unpause=encode_station_macid_pause_sleep(45,0,false);
 for(unsigned i=0;i<64;++i)assert(channel_u32(unpause.data()+8+4*i)==((i==4||i==12)?1u:0u));
 rejects([]{encode_station_macid_pause_sleep(0,128,false);});
 std::cout<<"CMAC G7/DMAC V2 complete word goldens and MACID bounds passed\n";
}
