#include "archer/driver_power_windows.hpp"
#include "archer/rf_power.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>
using Trace=std::vector<std::tuple<char,unsigned,unsigned,unsigned>>;
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0xf0,0x1000},{0x3f0,0x2200000},{0x1e0,0xc000000},{0x74,0x801004fd},{0,0xc29e},{0xcc,0x29},{0x18,0xabc00000}};
 std::map<unsigned,unsigned> x{{0x41,0},{0xfd,0x33},{0xfe,0x69},{0xe0,0},{0x90,0x90},{0x80,0xc6},{0x81,0xc6},{0x2d,0x30},{0x2e,0x30},{0xe1,0x52},{0xa1,0}};
 Trace trace;unsigned busy_reads=0,busy_remaining=0;bool bad_selector=false;
 unsigned read(unsigned a,unsigned width)override{auto v=r[a];if(a==0x270&&busy_remaining){--busy_remaining;v|=0x80000000u;}trace.emplace_back('r',a,width,v);return v;}
 void write(unsigned a,unsigned width,unsigned v)override{
  trace.emplace_back('w',a,width,v);
  if(a==0x270){const auto offset=v&255,mask=(v>>16)&255;
   if(((v>>24)&3)==1)r[a]=0x1000000|(x[offset]<<8)|(bad_selector&&offset==0xfd?offset^1:offset);
   else{x[offset]=(x[offset]&~mask)|((v>>8)&mask);r[a]=v&0x7fffffff;}
  }else r[a]=v;
  if(a==0x270)busy_remaining=busy_reads;
 }
 void pause()override{trace.emplace_back('p',0,0,0);}
 void body(){r[0]|=0x20;r[0xcc]|=0x80;r[0x18]|=1;x[0xe0]|=0x80;x[0x2d]|=2;}
};
struct Runner {
 std::uint64_t now=1;unsigned actions=0;bool injected=false;std::vector<unsigned> failures;
 bool one(archer::DriverPowerWindows& w,Bus& bus,int fail=-1,bool apply_uncertain=false){
  auto a=w.next(now);if(a.kind==archer::DriverFirmwareActionKind::none)return false;
  assert(w.next(now).kind==archer::DriverFirmwareActionKind::none);
  auto stale=a;++stale.epoch;assert(!w.complete(stale,0,0,0,now));
  const auto index=actions++;const bool error=int(index)==fail||std::find(failures.begin(),failures.end(),index)!=failures.end();
  if(error){injected=true;if(apply_uncertain&&a.kind==archer::DriverFirmwareActionKind::write)bus.write(a.address,a.width,a.value);assert(w.complete(a,-1,0,0,now));return true;}
  unsigned value=0,actual=a.width;
  if(a.kind==archer::DriverFirmwareActionKind::read)value=bus.read(a.address,a.width);
  else if(a.kind==archer::DriverFirmwareActionKind::write)bus.write(a.address,a.width,a.value);
  else {assert(a.kind==archer::DriverFirmwareActionKind::delay);assert(!w.complete(a,0,0,0,now));now=a.due_us;bus.pause();actual=0;}
  assert(w.complete(a,0,value,actual,now));assert(!w.complete(a,0,value,actual,now));return true;
 }
 void opening(archer::DriverPowerWindows& w,Bus& bus,int fail=-1,bool uncertain=false){for(unsigned n=0;n<3000&&!w.ready()&&!w.finished();++n)one(w,bus,fail,uncertain);}
 void restoring(archer::DriverPowerWindows& w,Bus& bus,int fail=-1,bool uncertain=false){for(unsigned n=0;n<3000&&!w.finished();++n)one(w,bus,fail,uncertain);assert(w.finished());}
};
int main(){
 unsigned full_actions=0,open_actions=0,fault_cases=0;std::array<unsigned,2> full_counts{},open_counts{};
 for(bool enabled:{false,true}){
  Bus reference,port;if(enabled){reference.r[0x74]|=1u<<28;port.r[0x74]|=1u<<28;}
  archer::with_wlan_io_window(reference,[&]{assert(archer::read_chip_identity(reference).analog_id==0x6933);archer::with_rf_power_window(reference,[&]{reference.body();});});
  archer::DriverPowerWindows w;assert(w.begin(1,1));Runner r;r.opening(w,port);assert(w.ready());open_actions=r.actions;
  assert(port.x[0xe0]==3&&port.x[0x90]==0x6f);port.body();assert(w.close(r.now,true));r.restoring(w,port);full_actions=r.actions;full_counts[enabled]=full_actions;open_counts[enabled]=open_actions;
  assert(w.restored()&&w.error()==archer::DriverWindowError::none&&reference.trace==port.trace&&reference.r==port.r&&reference.x==port.x);
  assert(!w.begin(2,r.now));
 }
 // Inject one failure at every action of both opening and restoration.
 Bus pristine;
 for(bool enabled:{false,true})for(unsigned fail=0;fail<full_counts[enabled];++fail)for(bool uncertain:{false,true}){
  ++fault_cases;Bus bus;if(enabled)bus.r[0x74]|=1u<<28;const auto original=bus;archer::DriverPowerWindows w;assert(w.begin(1,1));Runner r;r.opening(w,bus,int(fail),uncertain);
  if(w.ready())assert(w.close(r.now,false));r.restoring(w,bus,int(fail),uncertain);
  assert(r.injected);
  if(fail<open_counts[enabled]){assert(w.error()!=archer::DriverWindowError::none);assert(w.restored());assert(bus.x==original.x&&bus.r[0]==original.r.at(0)&&bus.r[0xcc]==original.r.at(0xcc)&&bus.r[0x18]==original.r.at(0x18));}
  else assert(w.restoration_error()!=archer::DriverWindowError::none&&!w.restored());
 }
 Bus bad;bad.x[0xfd]=0;archer::DriverPowerWindows unsupported;assert(unsupported.begin(2,1));Runner br;br.opening(unsupported,bad);br.restoring(unsupported,bad);assert(unsupported.error()==archer::DriverWindowError::prerequisite&&unsupported.restored());assert(!(bad.r[0x74]&(1u<<28)));
 Bus abandoned;archer::DriverPowerWindows lost;assert(lost.begin(3,1));Runner lr;lr.opening(lost,abandoned);assert(lost.ready());lost.abandon();assert(lost.finished()&&!lost.restored()&&lost.journal_entries()>0);
 Bus cancelled;archer::DriverPowerWindows cancel;assert(cancel.begin(4,1));Runner cr;
 while(!cancel.ready()&&cr.actions<10)cr.one(cancel,cancelled);
 auto pending=cancel.next(cr.now);cancel.request_cancel(cr.now);assert(cancel.next(cr.now).kind==archer::DriverFirmwareActionKind::none);
 assert(cancel.complete(pending,-1,0,0,cr.now));cr.restoring(cancel,cancelled);assert(cancel.error()==archer::DriverWindowError::cancelled&&cancel.restored());
 // Delayed SIO completion must preserve polling order and 10ms waits.
 Bus slow_ref,slow_port;slow_ref.busy_reads=slow_port.busy_reads=2;
 archer::with_wlan_io_window(slow_ref,[&]{assert(archer::read_chip_identity(slow_ref).analog_id==0x6933);archer::with_rf_power_window(slow_ref,[]{});});
 archer::DriverPowerWindows slow;assert(slow.begin(5,1));Runner sr;sr.opening(slow,slow_port);assert(slow.ready());assert(slow.close(sr.now,true));sr.restoring(slow,slow_port);assert(slow.restored()&&slow_ref.trace==slow_port.trace);
 Bus busy;busy.busy_reads=6;archer::DriverPowerWindows timed;assert(timed.begin(6,1));Runner tr;tr.opening(timed,busy);tr.restoring(timed,busy);assert(timed.error()==archer::DriverWindowError::poll_timeout&&timed.restored());
 Bus inactive;inactive.r[0x74]&=~(1u<<31);archer::DriverPowerWindows no_active;assert(no_active.begin(7,1));Runner ir;ir.opening(no_active,inactive);ir.restoring(no_active,inactive);assert(no_active.error()==archer::DriverWindowError::poll_timeout&&no_active.restored());
 Bus cleanup;archer::DriverPowerWindows multi;assert(multi.begin(8,1));Runner mr;mr.opening(multi,cleanup);assert(multi.ready());const auto open_count=mr.actions;mr.failures={open_count,open_count+3};assert(multi.close(mr.now,false));mr.restoring(multi,cleanup);assert(multi.error()==archer::DriverWindowError::body_failed&&multi.restoration_error()==archer::DriverWindowError::io_error&&multi.restoration_failure_mask());
 auto mask=multi.restoration_failure_mask();unsigned failed_entries=0;for(;mask;mask>>=1)failed_entries+=mask&1;assert(failed_entries==2&&!multi.restored()&&multi.journal_entries()==0);
 Bus expired;archer::DriverPowerWindows bounded;assert(bounded.begin(9,1));Runner er;er.opening(bounded,expired);assert(bounded.ready());assert(bounded.close(er.now,true));assert(bounded.next(er.now+30000000).kind==archer::DriverFirmwareActionKind::none);assert(bounded.finished()&&!bounded.restored()&&bounded.journal_entries()>0&&bounded.restoration_failure_mask());
 Bus late;archer::DriverPowerWindows waiting;assert(waiting.begin(10,1));auto outstanding=waiting.next(1);assert(waiting.next(30000001).kind==archer::DriverFirmwareActionKind::none&&!waiting.finished());assert(waiting.complete(outstanding,-1,0,0,30000001));Runner wr;wr.now=30000001;wr.restoring(waiting,late);assert(waiting.error()==archer::DriverWindowError::deadline&&waiting.restored());
 Bus wrong_reply;wrong_reply.bad_selector=true;archer::DriverPowerWindows wrong;assert(wrong.begin(11,1));Runner xr;xr.opening(wrong,wrong_reply);xr.restoring(wrong,wrong_reply);assert(wrong.error()==archer::DriverWindowError::xtal_reply&&wrong.restored());
 Bus body_error;archer::DriverPowerWindows body;assert(body.begin(12,1));Runner body_runner;body_runner.opening(body,body_error);assert(body.ready());body_error.r[0x84]=0x300007;assert(body.close(body_runner.now,false));body_runner.restoring(body,body_error);assert(body.error()==archer::DriverWindowError::body_failed&&body.restored()&&body_error.r[0x84]==0x300007);
 Bus held_bus;archer::DriverPowerWindows held;assert(held.begin(13,1));Runner hr;hr.opening(held,held_bus);assert(held.ready());const auto entries=held.journal_entries();held.request_cancel(hr.now);assert(held.ready()&&held.journal_entries()==entries&&held.next(hr.now).kind==archer::DriverFirmwareActionKind::none);assert(held.close(hr.now,false));hr.restoring(held,held_bus);assert(held.restored()&&held.error()==archer::DriverWindowError::cancelled);
 Bus split_ref,split_port;
 archer::with_wlan_io_window(split_ref,[&]{assert(archer::read_chip_identity(split_ref).analog_id==0x6933);archer::with_rf_power_window(split_ref,[]{});assert(split_ref.r[0x74]&(1u<<28));split_ref.read(0x1e0,4);});
 archer::DriverPowerWindows split;assert(split.begin(14,1));Runner sp;sp.opening(split,split_port);assert(split.ready());assert(split.close_rf(sp.now,true));
 for(unsigned n=0;n<3000&&!split.wlan_held();++n)sp.one(split,split_port);
 assert(split.wlan_held()&&!split.restored()&&(split_port.r[0x74]&(1u<<28))&&split_port.x==Bus{}.x);
 split_port.read(0x1e0,4);assert(split.finish_wlan(sp.now));sp.restoring(split,split_port);assert(split.restored()&&split_ref.trace==split_port.trace);
 Bus warm;warm.r[0x84]=7;archer::DriverPowerWindows cold_only;assert(cold_only.begin(15,1,true));Runner cp;cp.opening(cold_only,warm);cp.restoring(cold_only,warm);assert(cold_only.error()==archer::DriverWindowError::prerequisite&&cold_only.restored()&&warm.r[0x18]==0xabc00000);
 for(const auto& entry:warm.trace){const auto [kind,address,width,value]=entry;(void)width;if(kind=='w'&&address==0x270)assert(((value>>24)&3)==1);}
 Bus cold_ref,cold_port;
 archer::with_wlan_io_window(cold_ref,[&]{assert(archer::read_chip_identity(cold_ref).analog_id==0x6933);assert(!(cold_ref.read(0x84,4)&0x33)&&cold_ref.read(0x842c,4)==0);archer::with_rf_power_window(cold_ref,[]{});});
 archer::DriverPowerWindows cold_window;assert(cold_window.begin(16,1,true));Runner kr;kr.opening(cold_window,cold_port);assert(cold_window.ready());assert(cold_window.close(kr.now,true));kr.restoring(cold_window,cold_port);assert(cold_window.restored()&&cold_ref.trace==cold_port.trace);
 Bus wrong_cut;wrong_cut.r[0xf0]=0;archer::DriverPowerWindows cv;assert(cv.begin(17,1,true));Runner cv_runner;cv_runner.opening(cv,wrong_cut);cv_runner.restoring(cv,wrong_cut);assert(cv.error()==archer::DriverWindowError::prerequisite&&cv.restored());for(const auto& t:wrong_cut.trace)assert(!(std::get<0>(t)=='w'&&std::get<1>(t)==0x270));
 std::cout<<"Power windows: exact legacy open/restore traces, "<<fault_cases<<" injected/uncertain failures, analog identity, cancellation and disconnect passed\n";
}
