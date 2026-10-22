#include "archer/usb_rx_aggregation.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0x10398,0},{0x7880,7},{0x6000,0x12345678},{0x6004,0x42}};
 unsigned writes=0,fail=0;
 unsigned read(unsigned a,unsigned)override{return r.at(a);}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("after write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 Bus b;auto saved=b.r;archer::with_usb_rx_aggregation(b,[&]{assert(b.r[0x6000]==0x80ff2014&&b.r[0x6004]==0x1f);});assert(b.r==saved);
 rejects([&]{archer::with_usb_rx_aggregation(b,[]{throw std::runtime_error("callback");});});assert(b.r==saved);
 for(unsigned i=1;i<=2;++i){Bus f;f.fail=i;rejects([&]{archer::with_usb_rx_aggregation(f,[]{});});assert(f.r==saved);}
 Bus active;active.r[0x10398]=1;rejects([&]{archer::with_usb_rx_aggregation(active,[]{});});assert(!active.writes);
 Bus hci;hci.r[0x7880]=1;rejects([&]{archer::with_usb_rx_aggregation(hci,[]{});});assert(!hci.writes);
 std::cout<<"USB RX aggregation source profile and cleanup passed\n";
}
