#include "archer/single_probe.hpp"
#include <cassert>
#include <iostream>
int main(){
 assert(!archer::probe_report_terminal({1,1},8));
 assert(archer::probe_report_terminal({1,8},8));
 assert(archer::probe_report_terminal({0,1},8));
 assert(archer::probe_report_terminal({1,1},1));
 auto p=archer::single_probe_packet();assert(p.size()==96);
 assert(archer::channel_u32(p.data())==0x486480);
 assert(archer::channel_u32(p.data()+8)==0x240020);
 assert(archer::channel_u32(p.data()+12)==0x2001);
 assert(archer::channel_u32(p.data()+32)==0x410400);
 assert(p[64]==0x40&&p[88]==0&&p[89]==0&&p[90]==1&&p[91]==4);
 std::vector<std::uint8_t> rpt(80);archer::channel_put32(rpt.data(),1|(9<<2)|(1<<8));
 archer::channel_put32(rpt.data()+4,80);archer::channel_put32(rpt.data()+48,1<<12);
 archer::channel_put32(rpt.data()+56,1<<10);
 auto decoded=archer::single_probe_report(rpt.data(),rpt.size());assert(decoded&&decoded->state==0&&decoded->count==1);
 archer::channel_put32(rpt.data()+48,(1<<12)|(3<<8));
 decoded=archer::single_probe_report(rpt.data(),rpt.size());assert(decoded&&decoded->state==3);
 archer::channel_put32(rpt.data()+48,2<<12);assert(!archer::single_probe_report(rpt.data(),rpt.size()));
 archer::channel_put32(rpt.data(),1|(1<<8));assert(!archer::single_probe_report(rpt.data(),rpt.size()));
 bool bad=false;archer::channel_put32(rpt.data(),1|(9<<2)|(1<<8));archer::channel_put32(rpt.data()+4,60);
 try{(void)archer::single_probe_report(rpt.data(),60);}catch(const std::runtime_error&){bad=true;}assert(bad);
 std::cout<<"Fixed probe descriptor and report identity: OK\n";
}
