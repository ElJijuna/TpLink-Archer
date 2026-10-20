#include "archer/hwsi_window.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0x22adc,0x39004},{0x22bdc,0x39000}};
    unsigned writes=0;bool fail_second=false;
    unsigned read(unsigned a,unsigned)override{return r.at(a);}
    void write(unsigned a,unsigned,unsigned v)override{++writes;r[a]=v;if(fail_second&&writes==2)throw std::runtime_error("injected partial write");}
    void pause()override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    Bus b;const auto initial=b.r;bool ran=false;
    archer::with_hwsi_table_window(b,[&]{ran=true;assert(b.r.at(0x22adc)==0x08039000&&b.r.at(0x22bdc)==0x08039000);});
    assert(ran&&b.r==initial);
    rejects([&]{archer::with_hwsi_table_window(b,[&]{b.r[0x22adc]|=4;throw std::runtime_error("RF timeout");});});assert(b.r==initial);
    Bus partial;partial.fail_second=true;rejects([&]{archer::with_hwsi_table_window(partial,[]{assert(false);});});assert(partial.r==initial);
    Bus bad;bad.r[0x22bdc]=0xdeadbeef;rejects([&]{archer::with_hwsi_table_window(bad,[]{assert(false);});});assert(bad.writes==0);
    Bus c;c.r[0x20000]=0x1a;
    archer::with_bb_clock_window(c,[&]{assert(c.r.at(0x20000)==0x1e);});assert(c.r.at(0x20000)==0x1a);
    rejects([&]{archer::with_bb_clock_window(c,[]{throw std::runtime_error("timeout");});});assert(c.r.at(0x20000)==0x1a);
    c.r[0x20000]=0;const auto writes=c.writes;rejects([&]{archer::with_bb_clock_window(c,[]{assert(false);});});assert(c.writes==writes);
    std::cout<<"HWSI candidate controls: guarded entry and complete restoration OK\n";
}
