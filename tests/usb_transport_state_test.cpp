#include "archer/usb_transport_state.hpp"
#include "archer/usb_request.hpp"
#include <cassert>
#include <iostream>
int main(){
 archer::DriverRegisterSetup setup;
 for(auto address:{0u,0xfcu,0x1e0u,0x123456u,0xfffffcu})for(auto width:{1u,2u,4u})for(bool write:{false,true}){
  assert(archer::driver_register_setup(write,address,width,setup));
  const auto known=archer::register_request(write?archer::Direction::write:archer::Direction::read,address,width);
  assert(setup.type==known[0]&&setup.request==known[1]&&setup.value==(known[2]|known[3]<<8)&&setup.index==(known[4]|known[5]<<8)&&setup.length==known[6]);
 }
 for(auto width:{0u,3u,5u,0xffffffffu})assert(!archer::driver_register_setup(false,0xfc,width,setup));
 assert(!archer::driver_register_setup(false,0xffffff,4,setup));assert(!archer::driver_register_setup(true,0x1000000,1,setup));
 const std::uint8_t wire[4]={0x78,0x56,0x34,0x12};std::uint32_t value=0;
 for(auto width:{1u,2u,4u}){
  assert(archer::driver_register_finish(width,0,width,wire,value)==archer::RegisterCompletion::ok);
  assert(value==(width==1?0x78u:width==2?0x5678u:0x12345678u));
  value=0xdeadbeef;
  assert(archer::driver_register_finish(width,-7,width,wire,value)==archer::RegisterCompletion::usb_error&&value==0xdeadbeef);
  assert(archer::driver_register_finish(width,0,width-1,wire,value)==archer::RegisterCompletion::short_io&&value==0xdeadbeef);
  assert(archer::driver_register_finish(width,0,width+1,wire,value)==archer::RegisterCompletion::overrun&&value==0xdeadbeef);
 }
 assert(archer::driver_register_finish(4,0,4,nullptr,value)==archer::RegisterCompletion::bad_argument);
 archer::UsbTransportState exhausted(UINT64_MAX);assert(exhausted.activate(9));auto last=exhausted.reserve(0x84,512,1);assert(last.ticket==UINT64_MAX);
 assert(exhausted.complete(last,0,1)&&exhausted.begin_cancel(last.ticket)&&exhausted.cancel_confirmed(last));assert(!exhausted.reserve(0x84,512,1).ticket);
 archer::UsbTransportState state;assert(!state.reserve(0x84,512,200).ticket);assert(!state.activate(0));assert(state.activate(7));assert(!state.activate(8));
 for(auto ep:{0u,0x04u,0x85u,0xffu})assert(!state.reserve(ep,512,200).ticket);
 assert(!state.reserve(0x84,0,200).ticket);assert(!state.reserve(0x84,16385,200).ticket);assert(!state.reserve(0x84,512,0).ticket);assert(!state.reserve(0x84,512,2001).ticket);
 std::array<archer::TransferToken,12> tickets;
 for(auto& t:tickets){t=state.reserve(0x84,16384,200);assert(t.ticket);}
 assert(!state.reserve(0x84,512,200).ticket&&state.active()==12);
 auto first=tickets[0];assert(!state.complete({8,first.ticket,first.slot},0,512));
 assert(state.complete(first,0,7));assert(state.find(first.ticket)->actual==7);assert(!state.complete(first,0,9));
 assert(state.begin_cancel(first.ticket));assert(!state.complete(first,0,7));
 assert(!state.reserve(0x84,512,200).ticket); // Cannot recycle until Cancel handler confirms.
 assert(!state.cancel_confirmed({8,first.ticket,first.slot}));assert(state.cancel_confirmed(first));assert(!state.cancel_confirmed(first));
 auto replacement=state.reserve(0x09,100,200);assert(replacement.slot==first.slot&&replacement.ticket!=first.ticket);
 assert(!state.complete(first,0,100)&&!state.cancel_confirmed(first)); // Late callback never mutates replacement.
 assert(state.complete(replacement,0,99));assert(state.find(replacement.ticket)->kind==archer::TransferKind::short_write&&state.find(replacement.ticket)->actual==0);
 assert(state.complete(tickets[1],0,16385));assert(state.find(tickets[1].ticket)->kind==archer::TransferKind::overrun);
 assert(state.complete(tickets[2],-123,2));assert(state.find(tickets[2].ticket)->status==-123&&state.find(tickets[2].ticket)->actual==0);
 state.stop();assert(!state.running()&&!state.reserve(0x09,100,200).ticket);
 for(auto t:tickets)assert(!state.complete(t,0,100));
 assert(state.cancel_confirmed(replacement));for(unsigned i=1;i<tickets.size();++i)assert(state.cancel_confirmed(tickets[i]));
 assert(state.active()==0&&!state.activate(8));
 archer::UsbTransportState next;assert(next.activate(8));auto new_session=next.reserve(0x84,512,200);assert(!next.complete(first,0,3)&&next.complete(new_session,0,0));
 archer::UsbTransportState failed_submit;assert(failed_submit.activate(10));
 auto uncertain=failed_submit.reserve(0x05,512,200);assert(uncertain.ticket&&failed_submit.begin_cancel(uncertain.ticket));
 assert(!failed_submit.begin_cancel(uncertain.ticket));assert(!failed_submit.complete(uncertain,0,512));
 for(unsigned i=1;i<12;++i)assert(failed_submit.reserve(0x84,512,200).ticket);
 assert(!failed_submit.reserve(0x84,512,200).ticket); // Failed Cancel/no ack keeps the slot quarantined.
 assert(failed_submit.cancel_confirmed(uncertain));auto reused=failed_submit.reserve(0x05,512,200);
 assert(reused.ticket!=uncertain.ticket&&!failed_submit.complete(uncertain,0,512));
 assert(failed_submit.complete(reused,0,512)&&failed_submit.find(reused.ticket)->actual==512);
 std::cout<<"USB transport state: register differential replay, bounds/backpressure, short I/O, stale/duplicate callbacks and cancellation barriers passed\n";
}
