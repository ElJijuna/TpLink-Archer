#include "archer/interface_lifecycle.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
const archer::UnlinkedCamContext context{{2,1,2,3,4,5},0,0,0,0};
const std::array<unsigned,9> sequences{10,11,12,13,14,15,16,17,18};
std::vector<std::uint8_t> ack(const std::vector<std::uint8_t>& frame,bool done,unsigned status=0){
 auto identity=archer::channel_u32(frame.data());auto sequence=identity>>24;
 std::vector<std::uint8_t>b(12);archer::channel_put32(b.data(),1|(done?0x100:0));archer::channel_put32(b.data()+4,12);
 archer::channel_put32(b.data()+8,(identity&0xffff)|(done?(status<<16)|(sequence<<24):sequence<<16));return b;
}
void finish(archer::InterfaceContextLifecycle& s){
 auto policy=s.ack_policy();auto f=s.issue();rejects([&]{s.issue();});s.sent_successfully();
 if(policy!=archer::ContextAck::none){auto b=ack(f,policy==archer::ContextAck::done);assert(s.observe(b.data(),b.size()));}
}
int main(){
 using namespace archer;
 InterfaceContextLifecycle s(context,sequences,true,true,true);
 rejects([&]{s.begin_removal();});for(unsigned i=0;i<6;++i)finish(s);
 assert(s.phase()==ContextPhase::ready&&s.step()==6);rejects([&]{s.issue();});
 s.begin_removal();for(unsigned i=0;i<3;++i)finish(s);
 assert(s.phase()==ContextPhase::cleanup_acknowledged&&s.step()==9);rejects([&]{s.issue();});
 for(unsigned fail=0;fail<9;++fail){InterfaceContextLifecycle t(context,sequences,true,true,false);
   for(unsigned i=0;i<fail;++i){if(i==6)t.begin_removal();finish(t);}if(fail==6)t.begin_removal();
   t.issue();t.mark_transport_uncertain();assert(t.phase()==ContextPhase::uncertain);rejects([&]{t.issue();});rejects([&]{t.begin_removal();});}
 InterfaceContextLifecycle t(context,sequences,true,true,false);finish(t);
 auto f=t.issue();t.sent_successfully();auto received=ack(f,false);t.observe(received.data(),received.size());assert(t.step()==1);
 auto wrong=f;wrong[3]++;auto b=ack(wrong,true);assert(!t.observe(b.data(),b.size()));
 b=ack(f,true,7);t.observe(b.data(),b.size());assert(t.phase()==ContextPhase::uncertain);
 InterfaceContextLifecycle duplicate(context,sequences,true,true,false);auto first=duplicate.issue();duplicate.sent_successfully();
 b=ack(first,false);duplicate.observe(b.data(),b.size());assert(duplicate.step()==1);duplicate.observe(b.data(),b.size());assert(duplicate.step()==1);
 b=ack(first,true,3);duplicate.observe(b.data(),b.size());assert(duplicate.phase()==ContextPhase::uncertain);
 rejects([]{InterfaceContextLifecycle bad(context,sequences,false,true,false);});
 rejects([]{InterfaceContextLifecycle bad(context,sequences,true,false,false);});
 auto reused=sequences;reused[8]=reused[0];rejects([&]{InterfaceContextLifecycle bad(context,reused,true,true,false);});
 for(bool initial:{false,true}){
   InterfaceContextLifecycle restore(context,sequences,true,true,initial);
   for(unsigned i=0;i<6;++i)finish(restore);restore.begin_removal();finish(restore);finish(restore);
   auto final=restore.issue();assert(channel_u32(final.data()+8)==(initial?1u:0u));
   restore.sent_successfully();assert(restore.phase()==ContextPhase::removing);
   auto receipt=ack(final,false);restore.observe(receipt.data(),receipt.size());
   assert(restore.phase()==ContextPhase::cleanup_acknowledged);
 }
 InterfaceContextLifecycle malformed(context,sequences,true,true,false);malformed.issue();malformed.sent_successfully();
 std::vector<std::uint8_t> truncated(7);rejects([&]{malformed.observe(truncated.data(),truncated.size());});assert(malformed.phase()==ContextPhase::uncertain);
 std::cout<<"Context lifecycle ordering, ACK policies and uncertain-state guards passed\n";
}
