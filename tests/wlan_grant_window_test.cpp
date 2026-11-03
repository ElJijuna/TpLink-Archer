#include "archer/wlan_grant_window.hpp"
#include <map>
#include <cassert>
#include <iostream>
#include <sstream>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> values{{0x10398,0},{0x28104,0x80000000},{0x28204,0x80000000},{0xe348,0x01775774}};
 unsigned writes=0;
 std::ostream* read_log=nullptr;
 unsigned read(unsigned a,unsigned w)override{assert(w==4);if(read_log)*read_log<<"{\"operation\":\"read\"}\n";return values.at(a);}
 void write(unsigned a,unsigned w,unsigned v)override{assert(w==4&&a==0xe348);values[a]=v;++writes;}
 void pause()override{}
};
int main(){
 Bus logging;std::ostringstream lines;logging.read_log=&lines;
 archer::log_probe_grant_policy(logging,"enabled",lines);
 assert(lines.str()=="{\"operation\":\"read\"}\n{\"operation\":\"probe_grant_policy\",\"phase\":\"enabled\",\"value\":24598388}\n");
 Bus bus;bool called=false;
 archer::with_scc_wlan_grant_window(bus,[&]{called=true;const auto v=bus.values.at(0xe348);
  for(auto bit:{0u,1u,2u,3u,10u,11u,12u,13u})assert(v&(1u<<bit));
  for(auto bit:{4u,14u,16u})assert(v&(1u<<bit));
  for(auto bit:{5u,15u,17u})assert(!(v&(1u<<bit)));
  assert((v&~archer::scc_grant_mask)==(0x01775774&~archer::scc_grant_mask));
 });assert(called&&bus.writes==2&&bus.values.at(0xe348)==0x01775774);
 bool rejected=false;try{archer::with_scc_wlan_grant_window(bus,[]{throw std::runtime_error("stop");});}catch(...){rejected=true;}
 assert(rejected&&bus.values.at(0xe348)==0x01775774);
 for(auto a:{0x10398u,0x28104u,0x28204u,0xe348u}){
  Bus bad;bad.values[a]=1;bool ran=false;rejected=false;
  try{archer::with_scc_wlan_grant_window(bad,[&]{ran=true;});}catch(...){rejected=true;}
  assert(rejected&&!ran&&!bad.writes);
 }
 std::cout<<"Source v2 SCC WL/BT grants, preserved bits, restoration and admission guards passed\n";
}
