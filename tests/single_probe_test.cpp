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
 auto special=archer::special_channel9_probe_packet();assert(special.size()==104);
 assert(archer::channel_u32(special.data()+8)==0x240028);
 assert(archer::channel_u32(special.data()+12)==0x2002);
 assert(archer::channel_u32(special.data()+36)==0x20000000);
 assert(special[86]==0x20&&special[89]==8&&special[98]==1&&special[99]==4);
 assert(std::string(special.begin()+90,special.begin()+98)=="Especial");
 auto uni=archer::special_channel9_unicast_probe_packet();assert(uni.size()==104);
 const std::array<std::uint8_t,6> ap{0x50,0x92,0x6a,0x11,0x06,0x66};
 assert(std::equal(ap.begin(),ap.end(),uni.begin()+68));
 assert(std::equal(ap.begin(),ap.end(),uni.begin()+80));
 assert(uni[86]==0x30&&archer::channel_u32(uni.data()+12)==0x2003);
 assert(archer::channel_u32(uni.data()+36)==0x30000000);
 assert(archer::channel_u32(uni.data()+48)==0x88000000);
 assert(std::equal(uni.begin()+88,uni.end(),special.begin()+88));
 auto mon=archer::special_channel9_monitored_probe_packet();assert(mon.size()==104);
 assert(mon[86]==0x40&&archer::channel_u32(mon.data()+12)==0x2004);
 assert(archer::channel_u32(mon.data()+36)==0x40000000&&archer::channel_u32(mon.data()+48)==0x80000000);
 assert(std::equal(ap.begin(),ap.end(),mon.begin()+68));
 assert(std::equal(mon.begin()+88,mon.end(),uni.begin()+88));
 auto ack_probe=archer::special_channel9_ack_probe_packet();assert(ack_probe.size()==104);
 assert(ack_probe[86]==0x50&&archer::channel_u32(ack_probe.data()+12)==0x2005);
 assert(archer::channel_u32(ack_probe.data()+36)==0x50000000);
 assert(archer::channel_u32(ack_probe.data()+48)==0x80000000);
 auto external=archer::special_channel9_external_probe_packet();
 assert(external[86]==0x60&&archer::channel_u32(external.data()+12)==0x2006);
 assert(archer::channel_u32(external.data()+36)==0x60000000);
 for(std::size_t i=0;i<external.size();++i)
   if(i!=12&&i!=39&&i!=86)assert(external[i]==ack_probe[i]);
 auto phy=archer::special_channel9_phy_probe_packet();
 assert(phy[86]==0x70&&archer::channel_u32(phy.data()+12)==0x2007);
 assert(archer::channel_u32(phy.data()+36)==0x70000000);
 for(std::size_t i=0;i<phy.size();++i)
   if(i!=12&&i!=39&&i!=86)assert(phy[i]==external[i]);
 auto drained=archer::special_channel9_drained_phy_probe_packet();
 assert(drained[86]==0x80&&archer::channel_u32(drained.data()+12)==0x2008);
 assert(archer::channel_u32(drained.data()+36)==0x80000000);
 for(std::size_t i=0;i<drained.size();++i)
   if(i!=12&&i!=39&&i!=86)assert(drained[i]==phy[i]);
 auto grant=archer::special_channel9_grant_probe_packet();
 assert(grant[86]==0x90&&archer::channel_u32(grant.data()+12)==0x2009);
 assert(archer::channel_u32(grant.data()+36)==0x90000000);
 for(std::size_t i=0;i<grant.size();++i)
   if(i!=12&&i!=39&&i!=86)assert(grant[i]==drained[i]);
 std::vector<std::uint8_t> rpt(80);archer::channel_put32(rpt.data(),1|(9<<2)|(1<<8));
 archer::channel_put32(rpt.data()+4,80);archer::channel_put32(rpt.data()+48,1<<12);
 archer::channel_put32(rpt.data()+56,1<<10);
 auto decoded=archer::single_probe_report(rpt.data(),rpt.size());assert(decoded&&decoded->state==0&&decoded->count==1);
 archer::channel_put32(rpt.data()+48,(1<<12)|(3<<8));
 decoded=archer::single_probe_report(rpt.data(),rpt.size());assert(decoded&&decoded->state==3);
 archer::channel_put32(rpt.data()+48,2<<12);assert(!archer::single_probe_report(rpt.data(),rpt.size()));
 decoded=archer::single_probe_report(rpt.data(),rpt.size(),2);assert(decoded&&decoded->state==0);
 archer::channel_put32(rpt.data()+48,0);decoded=archer::single_probe_report(rpt.data(),rpt.size(),0);assert(decoded&&decoded->state==0&&decoded->count==1);assert(!archer::single_probe_report(rpt.data(),rpt.size(),1));
 archer::channel_put32(rpt.data()+48,1<<12);assert(!archer::single_probe_report(rpt.data(),rpt.size(),2));
 archer::channel_put32(rpt.data(),1|(1<<8));assert(!archer::single_probe_report(rpt.data(),rpt.size()));
 bool bad=false;archer::channel_put32(rpt.data(),1|(9<<2)|(1<<8));archer::channel_put32(rpt.data()+4,60);
 try{(void)archer::single_probe_report(rpt.data(),60);}catch(const std::runtime_error&){bad=true;}assert(bad);
 std::cout<<"Fixed probe descriptor and report identity: OK\n";
}
