#include "archer/phy_report_window.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0x10398,0},{0x10000,0x40000000},{0x11400,0x20f3e000},{0x11470,0xa0}};
 unsigned writes=0,fail=0;
 unsigned read(unsigned a,unsigned)override{return r[a];}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("after write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 Bus b;auto saved=b.r;archer::with_phy_report_window(b,[&]{assert(b.r[0x11400]==0x20f3e010&&b.r[0x11470]==0xa1);});assert(b.r==saved);
 rejects([&]{archer::with_phy_report_window(b,[]{throw std::runtime_error("callback");});});assert(b.r==saved);
 for(unsigned i=1;i<=2;++i){Bus f;f.fail=i;rejects([&]{archer::with_phy_report_window(f,[]{});});assert(f.r==saved);}
 Bus active;active.r[0x10398]=1;rejects([&]{archer::with_phy_report_window(active,[]{});});assert(!active.writes);
 std::cout<<"PHY report window restoration passed\n";
}
