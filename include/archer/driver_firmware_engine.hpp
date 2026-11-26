#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace archer {
struct DriverFirmwareFrame {const std::uint8_t* data=nullptr;std::uint32_t size=0;};
struct DriverFirmwareImage {const DriverFirmwareFrame* frames=nullptr;std::uint32_t count=0;};
enum class DriverFirmwareError {none,invalid_images,invalid_state,io_error,short_io,inaccessible,precondition,rom_rejected,poll_timeout,deadline,cancelled,invalid_clock};
enum class DriverFirmwareActionKind {none,read,write,bulk,delay};
struct DriverFirmwareAction {
 DriverFirmwareActionKind kind=DriverFirmwareActionKind::none;
 std::uint64_t epoch=0,ticket=0,due_us=0;
 std::uint32_t address=0,value=0,width=4;DriverFirmwareFrame frame{};
 std::uint8_t endpoint=7; // Measured firmware bulk OUT, not management OUT05.
};
// Port of boot_firmware(..., usb_without_dcpu=true), only the download core.
// Caller must first own the interface, bind immutable images, prepare WLAN/RF
// windows and fence the firmware epoch. This class cannot grant those policies.
class DriverFirmwareEngine {
 enum class Kind {check,nonzero,probe,write,rmw,poll,header,bodies,pause};
 struct Op {Kind kind=Kind::probe;std::uint32_t address=0,mask=0,bits=0,width=4;};
 std::array<Op,128> program_{};unsigned size_=0,pc_=0,polls_=0,body_=1;
 std::array<DriverFirmwareImage,2> images_{};
 std::uint64_t epoch_=0,next_ticket_=1,deadline_=0,last_time_=0;
 DriverFirmwareAction pending_{};DriverFirmwareError error_=DriverFirmwareError::none;
 bool begun_=false,done_=false,rmw_write_=false,poll_delay_=false,mutated_=false,reset_=false;
 std::uint32_t write_value_=0,frames_sent_=0;
 static std::uint32_t word(const std::uint8_t* p){return p[0]|std::uint32_t(p[1])<<8|std::uint32_t(p[2])<<16|std::uint32_t(p[3])<<24;}
 void add(Kind k,std::uint32_t a=0,std::uint32_t m=0,std::uint32_t b=0,unsigned w=4){
  if(size_==program_.size()){error_=DriverFirmwareError::invalid_state;return;}program_[size_++]={k,a,m,b,w};
 }
 void build(){
  add(Kind::check,0xfc,255,0x71);add(Kind::check,0xf0,0xf000,0x1000);add(Kind::check,0x3f0,0x3ff0000,0x2200000);
  add(Kind::check,0x74,0x90000000,0x90000000);add(Kind::check,0x84,0x33,0);add(Kind::check,0x842c,0xffffffff,0);
  add(Kind::check,0x7880,3,3);add(Kind::check,0xb000,0x7b0,0x430);add(Kind::check,0xb010,0x7fff,0);
  add(Kind::check,0x8420,0x1000,0x1000);add(Kind::check,0x8d00,3,3);add(Kind::check,0x9100,3,3);add(Kind::nonzero,0x904c,0xfff);
  for(auto a:{0x88u,0x188u,0x170u,0x40u,8u,0xd0u})add(Kind::probe,a);
  add(Kind::rmw,0x842c,0xffff,0x7ff9);
  add(Kind::rmw,0x84,2,0);add(Kind::rmw,0x84,1,0);add(Kind::rmw,0x84,2,2);add(Kind::rmw,0x84,4,0);
  add(Kind::rmw,0xd0,1u<<17,0);add(Kind::pause);
  constexpr std::array<std::array<std::uint32_t,2>,10> mcu{{{0x36990,0},{0x36994,0},{0x36998,0},{0x36820,0xfffffffe},{0x36800,0xc0000ffe},{0x36808,0x76543210},{0x36814,0xbfbfb000},{0x36818,0x0478c009},{0x36800,0xc0000fff},{0x36820,0xffffffff}}};
  for(const auto& pair:mcu)add(Kind::write,pair[0],0,pair[1]);
  add(Kind::rmw,0x188,0xffff,0);
  add(Kind::rmw,0x88,2,0);add(Kind::rmw,0x88,1u<<11,1u<<11);add(Kind::rmw,0x88,2,2);
  add(Kind::rmw,0x1e0,0x3fffffff,0); // Preserve only firmware run-environment bits.
  for(auto a:{0x1f0u,0x16cu,0x1f8u})add(Kind::write,a);
  add(Kind::rmw,0x1e0,0xa00,0xa00);
  for(auto a:{0x1f4u,0x1f8u,0x78f0u,0x168u,0x16cu,0x160u,0x164u})add(Kind::write,a);
  add(Kind::write,0x1a4,0,1u<<21); // W1C, never RMW.
  add(Kind::rmw,8,1u<<14,1u<<14);add(Kind::rmw,0x170,0x600,0);add(Kind::rmw,0x1e0,0x20100,0);
  add(Kind::write,0x184);add(Kind::rmw,0x40,1u<<19,0);add(Kind::rmw,0x1e0,0xf0,0x20);
  add(Kind::rmw,0x1e6,7,0,2);
  add(Kind::rmw,0x88,2,0);add(Kind::rmw,0x88,1u<<11,0);add(Kind::rmw,0x88,2,2);
  for(unsigned image=0;image<2;++image){
   if(!image)add(Kind::write,0x184,0,0x20248000);
   add(Kind::poll,0x1e0,2,2);add(Kind::header,image);add(Kind::poll,0x1e0,1,1);
   add(Kind::write,0x160);add(Kind::write,0x164);add(Kind::bodies,image);
   add(Kind::poll,0x1e0,image?0x800:0x200,0);
  }
  add(Kind::pause);add(Kind::poll,0x1e0,0x3c000000,3u<<26);
 }
 void fail(DriverFirmwareError error){if(error_==DriverFirmwareError::none)error_=error;pending_={};}
 bool clock(std::uint64_t now){
  if(now<last_time_){fail(DriverFirmwareError::invalid_clock);return false;}last_time_=now;
  if(now>=deadline_){fail(DriverFirmwareError::deadline);return false;}return true;
 }
public:
 static bool valid_images(const std::array<DriverFirmwareImage,2>& images){
  std::uint64_t bytes=0;
  for(const auto& image:images){
   if(!image.frames||image.count<2||image.count>512)return false;
   for(unsigned i=0;i<image.count;++i){const auto& f=image.frames[i];
    if(!f.data||f.size<25||f.size>16384||(word(f.data)&0x3fff)!=f.size-24||((word(f.data)>>24)&63)!=(i?14u:13u))return false;
    bytes+=f.size;if(bytes>8*1024*1024)return false;
   }
  }return true;
 }
 bool begin(const std::array<DriverFirmwareImage,2>& images,std::uint64_t epoch,std::uint64_t now){
  if(begun_)return false;begun_=true;
  if(!epoch||now>std::numeric_limits<std::uint64_t>::max()-30000000){fail(DriverFirmwareError::invalid_state);return false;}
  if(!valid_images(images)){fail(DriverFirmwareError::invalid_images);return false;}
  images_=images;epoch_=epoch;last_time_=now;deadline_=now+30000000;build();return error_==DriverFirmwareError::none;
 }
 DriverFirmwareError error()const{return error_;}
 bool done()const{return done_;}
 bool mutated()const{return mutated_;}
 bool cpu_reset_attempted()const{return reset_;}
 unsigned frames_sent()const{return frames_sent_;}
 void cancel(){if(begun_&&!done_)fail(DriverFirmwareError::cancelled);}
 DriverFirmwareAction next(std::uint64_t now){
  if(!begun_||done_||error_!=DriverFirmwareError::none)return {};
  if(!clock(now)||pending_.kind!=DriverFirmwareActionKind::none)return {};
  if(pc_==size_){done_=true;return {};}
  const auto& op=program_[pc_];DriverFirmwareAction a;
  a.epoch=epoch_;a.ticket=next_ticket_++;a.address=op.address;a.width=op.width;
  if(poll_delay_||op.kind==Kind::pause){a.kind=DriverFirmwareActionKind::delay;a.due_us=now+10000;}
  else if(op.kind==Kind::header||op.kind==Kind::bodies){a.kind=DriverFirmwareActionKind::bulk;a.frame=images_[op.address].frames[op.kind==Kind::header?0:body_];}
  else if(op.kind==Kind::write||(op.kind==Kind::rmw&&rmw_write_)){
   a.kind=DriverFirmwareActionKind::write;a.value=op.kind==Kind::write?op.bits:write_value_;mutated_=true;
   if(a.address==0x88)reset_=true; // Consumed before submission; no rollback/retry claim.
  }else a.kind=DriverFirmwareActionKind::read;
  pending_=a;return a;
 }
 bool complete(const DriverFirmwareAction& a,std::int32_t status,std::uint32_t value,std::uint32_t actual,std::uint64_t now){
  if(!begun_||done_||error_!=DriverFirmwareError::none||pending_.kind==DriverFirmwareActionKind::none||a.epoch!=epoch_||a.ticket!=pending_.ticket)return false;
  if(!clock(now))return false;
  const auto action=pending_;const auto op=program_[pc_];
  if(status){fail(DriverFirmwareError::io_error);return true;}
  if(action.kind==DriverFirmwareActionKind::delay&&now<action.due_us)return false;
  if(action.kind!=DriverFirmwareActionKind::delay&&actual!=(action.kind==DriverFirmwareActionKind::bulk?action.frame.size:action.width)){
   fail(DriverFirmwareError::short_io);return true;
  }
  pending_={};
  if(action.kind==DriverFirmwareActionKind::delay){
   if(poll_delay_){poll_delay_=false;if(polls_==100)fail(DriverFirmwareError::poll_timeout);}
   else ++pc_;
  }else if(action.kind==DriverFirmwareActionKind::bulk){
   ++frames_sent_;if(op.kind==Kind::header)++pc_;
   else if(++body_==images_[op.address].count){body_=1;++pc_;}
  }else if(action.kind==DriverFirmwareActionKind::write){rmw_write_=false;++pc_;}
  else{
   if(action.width==4&&(value==0xeaeaeaea||value==0xdeadbeef)){fail(DriverFirmwareError::inaccessible);return true;}
   if(op.kind==Kind::check||op.kind==Kind::nonzero){
    if(op.kind==Kind::check?(value&op.mask)!=op.bits:!(value&op.mask))fail(DriverFirmwareError::precondition);else ++pc_;
   }else if(op.kind==Kind::rmw){write_value_=(value&~op.mask)|(op.bits&op.mask);rmw_write_=true;}
   else if(op.kind==Kind::poll){
    const auto rom=(value>>26)&15;
    if(rom>=4&&rom<=7)fail(DriverFirmwareError::rom_rejected);
    else if((value&op.mask)==op.bits){polls_=0;++pc_;}
    else{++polls_;poll_delay_=true;}
   }else ++pc_;
  }
  return true;
 }
};
}
