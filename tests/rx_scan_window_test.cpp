#include "archer/rx_scan_window.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0x10000,0x40000080},{0x10398,0},{0x11424,0xffff},{0x1142c,0xffff},{0x11428,0xffff},{0x11420,0x0f1740be}};
 unsigned writes=0,fail=0;bool fired=false;
 unsigned read(unsigned a,unsigned)override{return r[a];}
 void write(unsigned a,unsigned,unsigned v)override{++writes;r[a]=v;if(writes==fail&&!fired){fired=true;throw std::runtime_error("injected after write");}}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 Bus b;auto original=b.r;
 archer::with_passive_rx_window(b,[&]{assert(!(b.r[0x10000]&0x80));assert(b.r[0x11424]==0&&b.r[0x1142c]==0&&b.r[0x11428]==0x120);assert((b.r[0x11420]&0x7bff)==4);});
 assert(b.r==original);
 archer::with_passive_rx_window(b,[&]{assert(b.r[0x11428]==0xffff);assert((b.r[0x11420]&0x7bff)==0);assert(b.r[0x11424]==0&&b.r[0x1142c]==0);},false,archer::RxAcceptance::all_management);
 assert(b.r==original);
 b.r[0x11400]=0x20f3e000;auto diagnostic_original=b.r;
 archer::with_passive_rx_window(b,[&]{assert(b.r[0x11400]==0x20f3e001);},true);
 assert(b.r==diagnostic_original);original=b.r;
 for(auto mode:{archer::RxAcceptance::broadcast_bit_clear,archer::RxAcceptance::sniffer}){
   archer::with_passive_rx_window(b,[&]{assert((b.r[0x11420]&0x7bff)==(mode==archer::RxAcceptance::sniffer?5u:0u));},false,mode);
   assert(b.r==original);
 }
 archer::with_passive_rx_window(b,[&]{assert(b.r[0x10000]&0x80);assert(!(b.r[0x10398]&0x3ffff));},false,archer::RxAcceptance::response_engine);
 assert(b.r==original);
 rejects([&]{archer::with_passive_rx_window(b,[]{throw std::runtime_error("receive failed");});});assert(b.r==original);
 for(unsigned i=1;i<=5;++i){Bus f;auto saved=f.r;f.fail=i;rejects([&]{archer::with_passive_rx_window(f,[]{});});assert(f.r==saved);}
 Bus sixth;sixth.r[0x11400]=0x20f3e000;auto saved=sixth.r;sixth.fail=6;
 rejects([&]{archer::with_passive_rx_window(sixth,[]{},true);});assert(sixth.r==saved);
 Bus active;active.r[0x10398]=1;rejects([&]{archer::with_passive_rx_window(active,[]{});});assert(!active.writes);
 std::cout<<"Passive RX window restoration tests passed\n";
}
