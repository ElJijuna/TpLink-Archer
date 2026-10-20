#include "archer/bb_table_window.hpp"
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0x20000,26},{0x22adc,0x39000}};unsigned writes=0;
 unsigned read(unsigned a,unsigned)override{return r.at(a);}void write(unsigned a,unsigned,unsigned v)override{++writes;r[a]=v;}void pause()override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){std::vector<std::pair<unsigned,unsigned>> table{{0x20000,30},{0x22adc,0x8039000},{0x20000,0x580801f}};Bus b;const auto initial=b.r;
 archer::with_bb_table_window(b,table,[&]{assert(b.r[0x20000]==0x580801f);});assert(b.r==initial);
 rejects([&]{archer::with_bb_table_window(b,table,[]{throw std::runtime_error("timeout");});});assert(b.r==initial);
 Bus bad;bad.r[0x22adc]=0xdeadbeef;rejects([&]{archer::with_bb_table_window(bad,table,[]{assert(false);});});assert(bad.writes==0);
 std::cout<<"Selected BB table baseline guards and restoration OK\n";}
