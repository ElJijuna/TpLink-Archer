#include "archer/interface_lifecycle.hpp"
#include <fstream>
#include <iterator>
#include <cassert>
#include <iostream>
void replay(unsigned first,const std::string& prefix){
 using namespace archer;
 const UnlinkedCamContext c{{2,0,0,0x89,0x22,0xa1},0,0,0,0};
 InterfaceContextLifecycle s(c,{first,first+1,first+2,first+3,first+4,first+5,first+6,first+7,first+8},true,true,false);
 unsigned fixture=0;
 for(unsigned step=0;step<9;++step){
   if(step==6){assert(s.phase()==ContextPhase::ready);s.begin_removal();}
   assert(s.step()==step);auto policy=s.ack_policy();s.issue();s.sent_successfully();
   if(policy==ContextAck::none){assert(s.step()==step+1);continue;}
   const auto path="tests/fixtures/"+prefix+"-rx-"+std::to_string(fixture++)+".bin";
   std::ifstream file(path,std::ios::binary);assert(file);
   std::vector<std::uint8_t>b{std::istreambuf_iterator<char>(file),{}};
   const auto packets=decode_usb_rx_be(b);assert(packets.size()==1);
   const auto& p=packets[0];assert(p.packet_type==10&&p.c2h&&p.payload_bytes==12);
   assert(p.c2h->category==1&&p.c2h->command_class==0);
   assert(p.c2h->function==(policy==ContextAck::receive?0u:1u));
   assert(s.observe(b.data()+p.payload_offset,p.payload_bytes)&&s.step()==step+1);
 }
 assert(fixture==8&&s.phase()==ContextPhase::cleanup_acknowledged);
}
int main(){replay(67,"context-ack-stage55");replay(76,"context-ack-stage55b");std::cout<<"Both physical stage55 context ACK replays passed; DMAC remains submission-only\n";}
