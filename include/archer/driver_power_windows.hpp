#pragma once
#include "archer/driver_firmware_engine.hpp"
namespace archer {
enum class DriverWindowError {none,invalid_state,io_error,short_io,inaccessible,prerequisite,xtal_busy,xtal_reply,poll_timeout,body_failed,cancelled,deadline,invalid_clock,disconnected};
// SIO microtask: every read/write/pause is completed by the owning dispatcher.
class DriverXtalTask {
 bool write_=false,done_=false,delay_=false;unsigned stage_=0,polls_=0,offset_=0,mask_=0,bits_=0,value_=0;
 DriverWindowError error_=DriverWindowError::none;
public:
 void read(unsigned offset){*this={};offset_=offset;}
 void write(unsigned offset,unsigned mask,unsigned bits){*this={};write_=true;stage_=1;offset_=offset;mask_=mask;bits_=bits;}
 bool done()const{return done_;}unsigned value()const{return value_;}DriverWindowError error()const{return error_;}
 DriverFirmwareAction next(std::uint64_t now)const{
  DriverFirmwareAction a;a.address=0x270;a.width=4;
  if(done_||error_!=DriverWindowError::none)return a;
  if(delay_){a.kind=DriverFirmwareActionKind::delay;a.due_us=now+10000;}
  else if(stage_==1){a.kind=DriverFirmwareActionKind::write;a.value=write_?0x80000000u|(mask_<<16)|(bits_<<8)|offset_:0x81000000u|offset_;}
  else a.kind=DriverFirmwareActionKind::read;
  return a;
 }
 void complete(const DriverFirmwareAction& a,std::uint32_t value){
  if(a.kind==DriverFirmwareActionKind::delay){delay_=false;if(polls_==6)error_=DriverWindowError::poll_timeout;return;}
  if(stage_==0){if(value&0x80000000u)error_=DriverWindowError::xtal_busy;else stage_=1;return;}
  if(stage_==1){stage_=2;return;}
  if(value&0x80000000u){++polls_;delay_=true;return;}
  if((value&255)!=offset_||((value>>24)&3)!=(write_?0u:1u)||(!write_&&((value>>28)&7))){error_=DriverWindowError::xtal_reply;return;}
  value_=(value>>8)&255;done_=true;
 }
};
class DriverPowerWindows {
 enum class Kind {check,probe,wlan_open,wlan_poll,xtal_check,xtal_edit,mmio,pause};
 struct Op {Kind kind=Kind::probe;std::uint32_t address=0,mask=0,bits=0,width=4;};
 struct Undo {Kind kind=Kind::probe;std::uint32_t address=0,mask=0,old=0,width=4;};
 enum class Phase {idle,opening,held,restoring,wlan_held,finished};
 std::array<Op,64> program_{};unsigned size_=0,pc_=0;
 std::array<Undo,32> journal_{};unsigned used_=0,stage_=0,polls_=0;
 DriverXtalTask xtal_;bool xtal_active_=false;
 Phase phase_=Phase::idle;std::uint64_t epoch_=0,ticket_=1,last_=0,deadline_=0;
 DriverFirmwareAction pending_{};
 DriverWindowError primary_=DriverWindowError::none,restore_=DriverWindowError::none;
 std::uint32_t old_=0,expected_=0,restore_failures_=0;bool cancel_pending_=false,deadline_pending_=false,changed_=false,hold_wlan_=false,wlan_owned_=false;
 static bool sentinel(std::uint32_t v){return v==0xeaeaeaea||v==0xdeadbeef;}
 void add(Kind k,unsigned a=0,unsigned m=0,unsigned b=0,unsigned w=4){
  if(size_==program_.size()){primary_=DriverWindowError::invalid_state;phase_=Phase::finished;return;}program_[size_++]={k,a,m,b,w};
 }
 void build(bool require_cold){
  add(Kind::check,0xfc,255,0x71);add(Kind::check,0x3f0,0x3ff0000,0x2200000);add(Kind::check,0x1e0,0x3c000000,3u<<26);
  add(Kind::wlan_open,0x74);add(Kind::wlan_poll,0x74);
  add(Kind::check,0xfc,255,0x71);add(Kind::check,0x3f0,0x3ff0000,0x2200000);add(Kind::check,0xf0,0xf000,0x1000);
  add(Kind::xtal_check,0x41,0,0);add(Kind::xtal_check,0xfd,255,0x33);add(Kind::xtal_check,0xfe,255,0x69);
  if(require_cold){add(Kind::check,0x84,0x33,0);add(Kind::check,0x842c,0xffffffff,0);}
  add(Kind::check,0xfc,255,0x71);add(Kind::check,0xf0,0xf000,0x1000);add(Kind::check,0x3f0,0x3ff0000,0x2200000);add(Kind::check,0x74,0x90000000,0x90000000);
  constexpr unsigned offsets[]{0xe0,0x90,0x80,0x81,0x2d,0x2e,0xe1,0xa1},baseline[]{0,0x90,0xc6,0xc6,0x30,0x30,0x52,0};
  for(unsigned i=0;i<8;++i)add(Kind::xtal_check,offsets[i],255,baseline[i]);
  add(Kind::xtal_edit,0xe0,2,2);add(Kind::xtal_edit,0xe0,1,1);
  add(Kind::mmio,0x18,0x40,0x40);add(Kind::xtal_edit,0x90,0x40,0x40);
  add(Kind::mmio,0x18,0x20,0x20);add(Kind::xtal_edit,0x90,0x20,0x20);
  add(Kind::xtal_edit,0x90,4,4);add(Kind::xtal_edit,0x90,8,8);add(Kind::xtal_edit,0x90,0x10,0);
  add(Kind::xtal_edit,0x80,255,0xeb);add(Kind::xtal_edit,0x81,255,0xeb);
  add(Kind::xtal_edit,0x90,1,1);add(Kind::xtal_edit,0x90,2,2);add(Kind::xtal_edit,0x90,0x80,0);
  add(Kind::xtal_edit,0x2d,0x40,0);add(Kind::xtal_edit,0x2e,0x40,0);add(Kind::xtal_edit,0xe1,0x60,0x40);add(Kind::xtal_edit,0xa1,2,0);
  add(Kind::mmio,0xcc,4,4,1);add(Kind::mmio,0,0x100,0x100,2);add(Kind::mmio,0,0x8000,0,2);add(Kind::pause);
  add(Kind::mmio,0,0x4000,0,2);add(Kind::mmio,0xcc,4,0,1);
 }
 void reset_op(){stage_=0;polls_=0;xtal_active_=false;}
 void restore_begin(std::uint64_t now){phase_=Phase::restoring;deadline_=now>std::numeric_limits<std::uint64_t>::max()-30000000?std::numeric_limits<std::uint64_t>::max():now+30000000;pending_={};reset_op();}
 void fail(DriverWindowError error,std::uint64_t now){
  pending_={};
  if(phase_==Phase::restoring){if(restore_==DriverWindowError::none)restore_=error;if(used_){restore_failures_|=1u<<(used_-1);--used_;}reset_op();}
  else{if(primary_==DriverWindowError::none)primary_=error;restore_begin(now);}
 }
 void advance(){if(phase_==Phase::restoring){if(used_)--used_;}else ++pc_;reset_op();}
 Op operation()const{
  if(phase_!=Phase::restoring)return program_[pc_];
  const auto& u=journal_[used_-1];return {u.kind,u.address,u.mask,u.old,u.width};
 }
 void mark_unrestored(){restore_failures_|=used_==32?0xffffffffu:((1u<<used_)-1);}
 bool save(const Op& op,std::uint32_t old){
  if(used_==journal_.size())return false;
  journal_[used_++]={op.kind,op.address,op.mask,old,op.width};return true;
 }
public:
 bool begin(std::uint64_t epoch,std::uint64_t now,bool require_cold=false){
  if(phase_!=Phase::idle||!epoch||now>std::numeric_limits<std::uint64_t>::max()-60000000)return false;
  epoch_=epoch;last_=now;deadline_=now+30000000;phase_=Phase::opening;build(require_cold);return primary_==DriverWindowError::none;
 }
 bool ready()const{return phase_==Phase::held;}
 bool wlan_held()const{return phase_==Phase::wlan_held;}
 bool finished()const{return phase_==Phase::finished;}
 bool restored()const{return finished()&&used_==0&&restore_==DriverWindowError::none;}
 bool changed()const{return changed_;}
 unsigned journal_entries()const{return used_;}
 std::uint32_t restoration_failure_mask()const{return restore_failures_;}
 DriverWindowError error()const{return primary_;}
 DriverWindowError restoration_error()const{return restore_;}
 bool close(std::uint64_t now,bool body_succeeded){
  if(phase_!=Phase::held||now<last_||now>std::numeric_limits<std::uint64_t>::max()-30000000)return false;
  hold_wlan_=false;last_=now;if(!body_succeeded&&primary_==DriverWindowError::none)primary_=DriverWindowError::body_failed;restore_begin(now);return true;
 }
 bool close_rf(std::uint64_t now,bool body_succeeded){
  if(!close(now,body_succeeded))return false;hold_wlan_=true;return true;
 }
 bool finish_wlan(std::uint64_t now){
  if(phase_!=Phase::wlan_held||now<last_||now>std::numeric_limits<std::uint64_t>::max()-30000000)return false;
  last_=now;hold_wlan_=false;restore_begin(now);return true;
 }
 void request_cancel(std::uint64_t now){
  if(phase_==Phase::finished||phase_==Phase::idle)return;
  if(phase_==Phase::restoring||phase_==Phase::held||phase_==Phase::wlan_held){if(primary_==DriverWindowError::none)primary_=DriverWindowError::cancelled;return;}
  if(now<last_)now=last_;
  if(pending_.kind!=DriverFirmwareActionKind::none&&pending_.kind!=DriverFirmwareActionKind::delay)cancel_pending_=true;
  else fail(DriverWindowError::cancelled,now);
 }
 void abandon(DriverWindowError reason=DriverWindowError::disconnected){
  if(phase_==Phase::idle||phase_==Phase::finished)return;
  if(reason==DriverWindowError::none)reason=DriverWindowError::invalid_state;
  if(primary_==DriverWindowError::none)primary_=reason;
  if(restore_==DriverWindowError::none)restore_=reason;mark_unrestored();pending_={};phase_=Phase::finished;
 }
 DriverFirmwareAction next(std::uint64_t now){
  if(phase_==Phase::idle||phase_==Phase::held||phase_==Phase::wlan_held||phase_==Phase::finished)return {};
  if(now<last_){if(pending_.kind!=DriverFirmwareActionKind::none)return {};fail(DriverWindowError::invalid_clock,last_);return {};}
  else last_=now;
  if(now>=deadline_){
   if(pending_.kind!=DriverFirmwareActionKind::none&&pending_.kind!=DriverFirmwareActionKind::delay){deadline_pending_=true;return {};}
   if(phase_==Phase::restoring){restore_=DriverWindowError::deadline;mark_unrestored();pending_={};phase_=Phase::finished;return {};}
   fail(DriverWindowError::deadline,now);
  }
  if(pending_.kind!=DriverFirmwareActionKind::none)return {};
  if(phase_==Phase::opening&&pc_==size_){phase_=Phase::held;return {};}
  if(phase_==Phase::restoring&&hold_wlan_&&used_==(wlan_owned_?1u:0u)){phase_=Phase::wlan_held;return {};}
  if(phase_==Phase::restoring&&!used_){phase_=Phase::finished;return {};}
  const auto op=operation();DriverFirmwareAction a;
  a.epoch=epoch_;a.ticket=ticket_++;a.address=op.address;a.width=op.width;
  if(op.kind==Kind::xtal_check||op.kind==Kind::xtal_edit){
   if(!xtal_active_){if(op.kind==Kind::xtal_edit&&phase_==Phase::restoring)stage_=1;xtal_.read(op.address);xtal_active_=true;}
   const auto sub=xtal_.next(now);a.kind=sub.kind;a.address=sub.address;a.width=sub.width;a.value=sub.value;a.due_us=sub.due_us;
  }else if(op.kind==Kind::pause||(op.kind==Kind::wlan_poll&&stage_==1)){
   a.kind=DriverFirmwareActionKind::delay;a.due_us=now+10000;
  }else if((op.kind==Kind::mmio||op.kind==Kind::wlan_open)&&stage_==1){a.kind=DriverFirmwareActionKind::write;a.value=expected_;changed_=true;}
  else a.kind=DriverFirmwareActionKind::read;
  if(a.kind==DriverFirmwareActionKind::write)changed_=true;
  pending_=a;return a;
 }
 bool complete(const DriverFirmwareAction& supplied,std::int32_t status,std::uint32_t value,unsigned actual,std::uint64_t now){
  if(phase_!=Phase::opening&&phase_!=Phase::restoring)return false;
  if(pending_.kind==DriverFirmwareActionKind::none||supplied.epoch!=epoch_||supplied.ticket!=pending_.ticket)return false;
  if(now<last_)return false;last_=now;
  const auto a=pending_;const auto op=operation();
  if(now>=deadline_)deadline_pending_=true;
  if(deadline_pending_){deadline_pending_=false;
   if(phase_==Phase::restoring){restore_=DriverWindowError::deadline;mark_unrestored();pending_={};phase_=Phase::finished;return true;}
   fail(DriverWindowError::deadline,now);return true;
  }
  if(cancel_pending_){cancel_pending_=false;fail(DriverWindowError::cancelled,now);return true;}
  if(status){fail(DriverWindowError::io_error,now);return true;}
  if(a.kind==DriverFirmwareActionKind::delay&&now<a.due_us)return false;
  if(a.kind!=DriverFirmwareActionKind::delay&&actual!=a.width){fail(DriverWindowError::short_io,now);return true;}
  if(a.kind==DriverFirmwareActionKind::read&&sentinel(value)){fail(DriverWindowError::inaccessible,now);return true;}
  pending_={};
  if(op.kind==Kind::xtal_check||op.kind==Kind::xtal_edit){
   if(a.kind==DriverFirmwareActionKind::write&&op.kind==Kind::xtal_edit)changed_=true;
   xtal_.complete(a,value);
   if(xtal_.error()!=DriverWindowError::none){fail(xtal_.error(),now);return true;}
   if(!xtal_.done())return true;
   if(op.kind==Kind::xtal_check){if((xtal_.value()&op.mask)!=op.bits)fail(DriverWindowError::prerequisite,now);else advance();}
   else if(stage_==0){
    if(!save(op,xtal_.value())){fail(DriverWindowError::invalid_state,now);return true;}
    stage_=1;xtal_.read(op.address);
   }else if(stage_==1){expected_=(xtal_.value()&~op.mask)|(op.bits&op.mask);stage_=2;xtal_.write(op.address,op.mask,op.bits);}
   else if(stage_==2){stage_=3;xtal_.read(op.address);}
   else if(xtal_.value()!=expected_)fail(DriverWindowError::xtal_reply,now);
   else advance();
  }else if(op.kind==Kind::check){if((value&op.mask)!=op.bits)fail(DriverWindowError::prerequisite,now);else advance();}
  else if(op.kind==Kind::probe||op.kind==Kind::pause)advance();
  else if(op.kind==Kind::wlan_open){
   if(stage_==0){
    if(value&(1u<<28))advance();
    else{Op undo{Kind::mmio,0x74,1u<<28,0,4};if(!save(undo,value)){fail(DriverWindowError::invalid_state,now);return true;}
     wlan_owned_=true;expected_=value|(1u<<28);stage_=1;}
   }else advance();
  }else if(op.kind==Kind::wlan_poll){
   if(a.kind==DriverFirmwareActionKind::delay){stage_=0;if(polls_==20)fail(DriverWindowError::poll_timeout,now);}
   else if(value&(1u<<31))advance();else{++polls_;stage_=1;}
  }else if(op.kind==Kind::mmio){
   if(stage_==0){old_=value;
    if(phase_!=Phase::restoring&&!save(op,old_)){fail(DriverWindowError::invalid_state,now);return true;}
    expected_=(old_&~op.mask)|(op.bits&op.mask);stage_=1;
   }else if(stage_==1)stage_=2;
   else if((value&op.mask)!=(op.bits&op.mask))fail(DriverWindowError::prerequisite,now);
   else advance();
  }
  return true;
 }
};
}
