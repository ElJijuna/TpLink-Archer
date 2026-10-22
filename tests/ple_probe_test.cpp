#include "archer/ple_probe.hpp"
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus{
 unsigned ctl=0x10002,data=0x12345678,writes=0;bool stuck=false,fail_data=false;
 unsigned read(unsigned a,unsigned)override{if(a==0x9110)return ctl;if(fail_data)throw std::runtime_error("data read");return data;}
 void write(unsigned,unsigned,unsigned v)override{++writes;ctl=stuck?v:v&~0x80000000u;}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 Bus b;assert(archer::ple_debug_read(b,7,0)==b.data&&b.ctl==0x10002&&b.writes==2);
 b.fail_data=true;rejects([&]{archer::ple_debug_read(b,1,6);});assert(b.ctl==0x10002);
 Bus busy;busy.ctl|=0x80000000;rejects([&]{archer::ple_debug_read(busy,7,0);});assert(!busy.writes);
 Bus bad;rejects([&]{archer::ple_debug_read(bad,0,0);});assert(!bad.writes);
 Bus stuck;stuck.stuck=true;rejects([&]{archer::ple_debug_read(stuck,7,0);});assert(stuck.writes==1&&(stuck.ctl&0x80000000));
 std::cout<<"PLE debug guards, completion and cleanup tests passed\n";
}
