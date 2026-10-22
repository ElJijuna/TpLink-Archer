#include "archer/port_rx_window.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0x10000,0x40000000},{0x10398,0},{0x10400,0x60018}};
 unsigned writes=0,fail=0;
 unsigned read(unsigned a,unsigned)override{return r.at(a);}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("after write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 Bus b;auto saved=b.r;archer::with_unlinked_port0_window(b,[&]{assert(b.r[0x10400]==0x60004);});assert(b.r==saved);
 rejects([&]{archer::with_unlinked_port0_window(b,[]{throw std::runtime_error("callback");});});assert(b.r==saved);
 Bus failed;failed.fail=1;rejects([&]{archer::with_unlinked_port0_window(failed,[]{});});assert(failed.r==saved);
 for(unsigned bit:{2u,9u,10u,11u,12u}){Bus owned;owned.r[0x10400]|=1u<<bit;rejects([&]{archer::with_unlinked_port0_window(owned,[]{});});assert(!owned.writes);}
 std::cout<<"Unlinked port0 guards and restoration passed\n";
}
