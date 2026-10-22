#include "archer/rx_counters.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0x20540,0x123480},{0x20730,0xabc002},{0x10398,0}};
 unsigned writes=0,fail=0;
 unsigned read(unsigned a,unsigned)override{return r[a];}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("after write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 for(unsigned i=0;i<=4;++i){Bus b;auto saved=b.r;b.fail=i;if(i)rejects([&]{archer::reset_rx_diagnostic_counters(b);});else archer::reset_rx_diagnostic_counters(b);assert(saved==b.r);}
 Bus active;active.r[0x10398]=1;rejects([&]{archer::reset_rx_diagnostic_counters(active);});assert(!active.writes);
 Bus held;held.r[0x20730]|=1;rejects([&]{archer::reset_rx_diagnostic_counters(held);});assert(!held.writes);
 std::cout<<"PMAC counter reset guards and failure cleanup passed\n";
}
