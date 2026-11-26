#include "archer/driver_firmware_engine.hpp"
#include "archer/firmware_boot.hpp"
#include "BoundFirmware.hpp"
#include <cassert>
#include <map>
#include <tuple>
#include <vector>
#include <iostream>
using Trace=std::vector<std::tuple<char,unsigned,unsigned,unsigned>>;
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> r{{0xfc,0x371},{0xf0,0x1000},{0x3f0,0x2200000},{0x74,0x901004fd},{0x7880,3},{0xb000,0x430},{0xb010,0x80000000},{0x8420,0x1000},{0x84,0},{0x842c,0},{0x8d00,3},{0x9100,3},{0x904c,0x100},{0x1e0,0x0c000000}};
 Trace trace;std::vector<std::vector<std::uint8_t>> packets;unsigned sent=0;bool ready=true,security=false;
 unsigned read(unsigned a,unsigned w)override{const auto v=r[a];trace.emplace_back('r',a,w,v);return v;}
 void write(unsigned a,unsigned w,unsigned v)override{trace.emplace_back('w',a,w,v);r[a]=v;if(a==0x88&&v==2&&ready&&r[0x1e0]==0xa20)r[0x1e0]|=2;}
 void pause()override{trace.emplace_back('p',0,0,0);}
 void send(const std::vector<std::uint8_t>& f){
  packets.push_back(f);++sent;trace.emplace_back('b',unsigned(f.size()),0,0);
  if(security){r[0x1e0]=5u<<26;return;}
  if((f[3]&63)==13)r[0x1e0]|=1;
  else if(sent==2)r[0x1e0]&=~0x200u;else r[0x1e0]=0x0c000003;
 }
};
std::vector<std::uint8_t> frame(unsigned type){std::vector<std::uint8_t> f(32);archer::channel_put32(f.data(),type<<24|8);return f;}
struct Images {
 std::array<std::vector<std::uint8_t>,2> data{frame(13),frame(14)};
 std::array<archer::DriverFirmwareFrame,2> frames{{{data[0].data(),32},{data[1].data(),32}}};
 std::array<archer::DriverFirmwareImage,2> views{{{frames.data(),2},{frames.data(),2}}};
};
unsigned run(archer::DriverFirmwareEngine& engine,Bus& bus,int fail_at=-1,bool short_io=false){
 std::uint64_t now=1;unsigned count=0;
 while(!engine.done()&&engine.error()==archer::DriverFirmwareError::none){
  const auto a=engine.next(now);if(a.kind==archer::DriverFirmwareActionKind::none)break;
  assert(engine.next(now).kind==archer::DriverFirmwareActionKind::none); // No duplicate issuance while pending.
  auto stale=a;stale.epoch+=1;assert(!engine.complete(stale,0,0,0,now));
  if(int(count)==fail_at){engine.complete(a,short_io?0:-123,0,0,now);++count;break;}
  unsigned value=0,actual=a.width;
  switch(a.kind){
   case archer::DriverFirmwareActionKind::read:value=bus.read(a.address,a.width);break;
   case archer::DriverFirmwareActionKind::write:bus.write(a.address,a.width,a.value);break;
   case archer::DriverFirmwareActionKind::bulk:bus.send({a.frame.data,a.frame.data+a.frame.size});actual=a.frame.size;break;
   case archer::DriverFirmwareActionKind::delay:assert(!engine.complete(a,0,0,0,now));now=a.due_us;bus.pause();actual=0;break;
   default:assert(false);
  }
  assert(engine.complete(a,0,value,actual,now));assert(!engine.complete(a,0,value,actual,now));++count;
 }
 return count;
}
int main(){
 Images images;const std::vector<archer::BootImage> old(2,archer::BootImage{{images.data[0],images.data[1]}});
 Bus reference;archer::boot_firmware(reference,old,[&](const auto& f){reference.send(f);},[](const char*){},true);
 Bus port;archer::DriverFirmwareEngine engine;assert(engine.begin(images.views,9,1));const auto steps=run(engine,port);
 assert(engine.done()&&engine.frames_sent()==4&&engine.cpu_reset_attempted());assert(reference.trace==port.trace&&reference.r==port.r);
 assert(!engine.begin(images.views,10,1));
 for(unsigned i=0;i<steps;++i){
  archer::DriverFirmwareEngine failed;assert(failed.begin(images.views,9,1));Bus bus;
  run(failed,bus,int(i));assert(failed.error()==archer::DriverFirmwareError::io_error);
  const auto trace=bus.trace;assert(failed.next(1000).kind==archer::DriverFirmwareActionKind::none&&bus.trace==trace);
 }
 for(bool ready:{false,true}){
  Bus a,b;a.ready=b.ready=ready;a.security=b.security=ready;
  bool caught=false;try{archer::boot_firmware(a,old,[&](const auto& f){a.send(f);},[](const char*){},true);}catch(const std::runtime_error&){caught=true;}assert(caught);
  archer::DriverFirmwareEngine f;assert(f.begin(images.views,9,1));run(f,b);
  assert(f.error()==(ready?archer::DriverFirmwareError::rom_rejected:archer::DriverFirmwareError::poll_timeout));assert(a.trace==b.trace);
 }
 Bus blocked;blocked.r[0x8d00]=0;archer::DriverFirmwareEngine pre;assert(pre.begin(images.views,9,1));run(pre,blocked);assert(pre.error()==archer::DriverFirmwareError::precondition&&!pre.mutated());
 archer::DriverFirmwareEngine malformed;auto bad=images.views;bad[1].count=1;assert(!malformed.begin(bad,9,1)&&malformed.error()==archer::DriverFirmwareError::invalid_images);
 archer::DriverFirmwareEngine deadline;assert(deadline.begin(images.views,9,1));auto pending=deadline.next(1);assert(!deadline.complete(pending,0,0x371,4,30000001));assert(deadline.error()==archer::DriverFirmwareError::deadline);
 archer::DriverFirmwareEngine cancelled;assert(cancelled.begin(images.views,9,1));pending=cancelled.next(1);cancelled.cancel();assert(!cancelled.complete(pending,0,0x371,4,2));
 archer::DriverFirmwareEngine shorted;assert(shorted.begin(images.views,9,1));Bus sb;run(shorted,sb,0,true);assert(shorted.error()==archer::DriverFirmwareError::short_io&&!shorted.mutated());
 auto check_bad=[&](std::array<archer::DriverFirmwareImage,2> changed){archer::DriverFirmwareEngine invalid;assert(!invalid.begin(changed,9,1)&&!invalid.mutated());};
 bad=images.views;bad[0].frames=nullptr;check_bad(bad);
 bad=images.views;bad[0].count=513;check_bad(bad);
 auto changed_frames=images.frames;changed_frames[0].data=nullptr;bad=images.views;bad[0].frames=changed_frames.data();check_bad(bad);
 changed_frames=images.frames;changed_frames[0].size=16385;bad[0].frames=changed_frames.data();check_bad(bad);
 changed_frames=images.frames;changed_frames[0].size=31;bad[0].frames=changed_frames.data();check_bad(bad);
 changed_frames=images.frames;changed_frames[0]=images.frames[1];bad[0].frames=changed_frames.data();check_bad(bad);
 archer::DriverFirmwareEngine clock;assert(clock.begin(images.views,9,100));assert(clock.next(99).kind==archer::DriverFirmwareActionKind::none&&clock.error()==archer::DriverFirmwareError::invalid_clock);
 std::vector<archer::BootImage> retained;
 for(const auto& image:driver_bound_firmware){
  archer::BootImage bundle;
  for(unsigned i=0;i<image.count;++i)bundle.frames.push_back({image.frames[i].data,image.frames[i].data+image.frames[i].size});
  retained.push_back(std::move(bundle));
 }
 Bus full_ref,full_port;archer::boot_firmware(full_ref,retained,[&](const auto& f){full_ref.send(f);},[](const char*){},true);
 archer::DriverFirmwareEngine full;assert(full.begin(driver_bound_firmware,11,1));run(full,full_port);
 assert(full.done()&&full.frames_sent()==212&&full_ref.trace==full_port.trace&&full_ref.packets==full_port.packets);
 unsigned last_bulk=0;for(unsigned i=0;i<full_port.trace.size();++i)if(std::get<0>(full_port.trace[i])=='b')last_bulk=i;
 archer::DriverFirmwareEngine late;assert(late.begin(driver_bound_firmware,11,1));Bus late_bus;run(late,late_bus,int(last_bulk),true);
 assert(late.error()==archer::DriverFirmwareError::short_io&&late.frames_sent()==211&&late.next(1000000).kind==archer::DriverFirmwareActionKind::none);
 std::cout<<"Driver firmware core: exact legacy trace, "<<steps<<" injected failures, all 212 bound packets, ROM/timeout, short I/O, cancellation and deadline passed\n";
}
