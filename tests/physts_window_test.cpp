#include "archer/physts_window.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0x10398,0},{0x10000,0x40000000},{0x20738,0xa0000}};
 unsigned writes=0,fail=0;
 Bus(){for(unsigned page=0;page<=16;++page)if(page!=9)r[archer::rtl8922a_physts_address(page)]=0xabcdef01;}
 unsigned read(unsigned a,unsigned)override{return r.at(a);}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("after write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 using namespace archer;
 assert(rtl8922a_physts_address(8)==0x2075c&&rtl8922a_physts_address(10)==0x20760&&rtl8922a_physts_address(16)==0x20788);
 rejects([]{rtl8922a_physts_address(9);});rejects([]{rtl8922a_physts_address(17);});
 assert(rtl8922a_physts_value(6,0)==0x2000&&rtl8922a_physts_value(10,0)==0x2002);
 assert(rtl8922a_physts_value(11,0xff)==0xf&&rtl8922a_physts_value(12,0xff)==0xf);
 assert(rtl8922a_physts_value(13,0xff)==0x10000f&&rtl8922a_physts_value(16,0xff)==0x10000f);
 Bus b;auto saved=b.r;with_physts_window(b,[&]{assert(b.r.at(0x20738)==0xa000c);assert(b.r.at(0x20764)==0xabcdef03);});assert(b.r==saved);
 rejects([&]{with_physts_window(b,[]{throw std::runtime_error("callback");});});assert(b.r==saved);
 for(unsigned i=1;i<=17;++i){Bus f;f.fail=i;rejects([&]{with_physts_window(f,[]{});});assert(f.r==saved);}
 Bus active;active.r[0x10398]=1;rejects([&]{with_physts_window(active,[]{});});assert(!active.writes);
 std::cout<<"PHY status page mapping and journal tests passed\n";
}
