#include "archer/phy_tx_counters.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> values{{0x20540,0x80},{0x20730,2},
  {0x22e00,0x12345678},{0x22e04,0xabcd4321},{0x20e40,0x56788765}};
 unsigned fail=0,reads=0,writes=0;
 unsigned read(unsigned address,unsigned width)override{
  assert(width==4);if(++reads==fail)throw std::runtime_error("short transfer");
  return values.at(address);
 }
 void write(unsigned,unsigned,unsigned)override{++writes;throw std::runtime_error("unexpected write");}
 void pause()override{}
};
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed);}
int main(){
 Bus b;auto before=archer::read_phy_tx_snapshot(b);
 assert(before.enabled());assert((before.counts()==std::array<unsigned,4>{0x1234,0x4321,0x5678,0x8765}));
 auto after=before;after.cck_txen_raw+=1u<<16;after.cck_txon_raw+=2;
 assert((archer::phy_tx_modulo_delta(before,after)==std::array<unsigned,4>{1,2,0,0}));
 before.cck_txen_raw=0xffff0000;after.cck_txen_raw=0;
 assert(archer::phy_tx_modulo_delta(before,after)[0]==1);
 for(unsigned i=0;i<2;++i){auto held=before;if(i)held.all_control=3;else held.cck_control=0;
  rejects([&]{(void)archer::phy_tx_modulo_delta(held,after);});
  rejects([&]{(void)archer::phy_tx_modulo_delta(after,held);});
 }
 for(unsigned i=1;i<=5;++i){Bus failed;failed.fail=i;
  rejects([&]{(void)archer::read_phy_tx_snapshot(failed);});assert(!failed.writes);
 }
 assert(!b.writes);std::cout<<"PHY TX field decoding, modulo/disabled guards and read failures passed\n";
}
