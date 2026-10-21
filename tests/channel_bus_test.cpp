#include "archer/channel_bus.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> regs{{0xfc,0x71},{0x10000,0x40000000},{0x10398,0x123ffff},{0x11440,0x6d}};
    unsigned writes=0,drop=0;
    unsigned read(unsigned a,unsigned)override{return regs[a];}
    void write(unsigned a,unsigned,unsigned v)override{++writes;if(a!=drop)regs[a]=v;}
    void pause()override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    using namespace archer;
    Bus bus;auto saved=pause_cmac0_scheduler(bus);assert(saved==0x123ffff && bus.regs[0x10398]==0x1200000);
    resume_cmac0_scheduler(bus,saved);assert(bus.regs[0x10398]==saved);
    set_cmac0_ppdu(bus,false);assert(bus.regs[0x11440]==0x6c);
    set_cmac0_ppdu(bus,true);assert(bus.regs[0x11440]==0x6d && bus.regs[0x9c30]==3);
    Bus missing;missing.regs[0x10000]=0;rejects([&]{pause_cmac0_scheduler(missing);});assert(!missing.writes);
    unsigned delays=0,reads=0;
    wait_rf_not_transmitting([&](unsigned,unsigned){return ++reads<3?0x20000u:0x10000u;},[&](unsigned us){assert(us==2);++delays;},[]{return false;});
    assert(delays==2 && reads==4);
    rejects([&]{wait_rf_not_transmitting([](unsigned,unsigned){return 0x20000u;},[](unsigned){},[]{return true;});});
    rejects([&]{wait_rf_not_transmitting([](unsigned,unsigned){return 0xdeadbeefu;},[](unsigned){},[]{return true;});});
    ChannelBus coordinator(bus,[](unsigned){});
    coordinator.execute(channel1_reset_enter({}));assert(coordinator.paused() && !(bus.regs[0x10398]&0x3ffff));
    coordinator.execute(channel1_reset_exit({}));assert(coordinator.paused());
    rejects([&]{coordinator.execute(channel1_reset_enter({}));});
    Bus ignored;ignored.drop=0x10398;rejects([&]{pause_cmac0_scheduler(ignored);});
    std::cout<<"Channel bus scheduler, PPDU, reset ownership and bounded RF mode checks OK\n";
}
