#include "archer/usb_service_lifecycle.hpp"
#include <cassert>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
struct Backend {
 static constexpr std::int32_t invalid_state=-99;
 int fail=-1;bool opened=false;std::array<bool,9> pipes{};
 std::vector<std::string> events;
 archer::UsbServiceLifecycle<Backend>* owner=nullptr;
 int open(){events.push_back("open");if(fail==0)return -10;assert(!opened);opened=true;return 0;}
 int validate(){events.push_back("validate");return fail==1?-11:0;}
 int copy_pipe(unsigned i,std::uint8_t endpoint){
  events.push_back("copy"+std::to_string(i));assert(opened&&endpoint==archer::service_endpoints[i]);
  if(fail==int(i)+2)return -12;assert(!pipes[i]);pipes[i]=true;return 0;
 }
 int abort_control(){
  events.push_back("abort-control");assert(!owner->accepts_io());
  assert(owner->start()==invalid_state);assert(owner->stop()==0); // Reentrant cancellation cannot restart or double-close.
  return fail==11?-21:0;
 }
 int abort_pipe(unsigned i){events.push_back("abort"+std::to_string(i));assert(pipes[i]&&!owner->accepts_io());return fail==12?-22:0;}
 int close(){events.push_back("close");assert(opened);opened=false;return fail==13?-23:0;}
 void release_pipe(unsigned i){events.push_back("release"+std::to_string(i));assert(!opened&&pipes[i]);pipes[i]=false;}
};
int main(){
 for(int fail=-1;fail<=13;++fail){
  Backend backend;backend.fail=fail;archer::UsbServiceLifecycle<Backend> life(backend);backend.owner=&life;
  assert(!life.accepts_io());auto result=life.start();
  if(fail>=0&&fail<=10){assert(result<0);assert(life.state()==archer::UsbServiceState::stopped);}
  else{assert(result==0&&life.accepts_io());assert(life.start()==Backend::invalid_state);result=life.stop();assert((fail>=11)==(result<0));}
  assert(!life.accepts_io()&&!backend.opened);
  for(bool owned:backend.pipes)assert(!owned);
  auto events=backend.events;assert(life.stop()==0&&events==backend.events);
  assert(life.start()==Backend::invalid_state);
  assert(std::count(events.begin(),events.end(),"close")== (fail==0?0:1));
  const unsigned acquired=fail==0||fail==1?0:fail>=2&&fail<=10?unsigned(fail-2):9;
  for(unsigned i=0;i<9;++i){
   assert(std::count(events.begin(),events.end(),"abort"+std::to_string(i))==int(i<acquired));
   assert(std::count(events.begin(),events.end(),"release"+std::to_string(i))==int(i<acquired));
  }
 }
 Backend unused;archer::UsbServiceLifecycle<Backend> idle(unused);unused.owner=&idle;assert(idle.stop()==0&&unused.events.empty());
 std::ifstream input("tests/fixtures/37ad-0104-usb2-config.bin",std::ios::binary);
 std::vector<std::uint8_t> config((std::istreambuf_iterator<char>(input)),{});assert(config.size()==81);
 const auto valid=[&](const auto& bytes){return archer::service_usb2_profile(0x37ad,0x0104,bytes.data(),bytes.size());};
 assert(valid(config));
 assert(!archer::service_usb2_profile(0x0bda,0x1a2b,config.data(),config.size()));
 assert(!archer::service_usb2_profile(0x37ad,0x0105,config.data(),config.size()));
 assert(!archer::service_usb2_profile(0x37ad,0x0104,nullptr,config.size()));
 for(unsigned n=0;n<config.size();++n)assert(!archer::service_usb2_profile(0x37ad,0x0104,config.data(),n));
 for(unsigned offset:{0,1,2,4,5,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24}){auto changed=config;changed[offset]^=1;assert(!valid(changed));}
 auto duplicate=config;duplicate[27]=duplicate[20];assert(!valid(duplicate));
 std::cout<<"USB lifecycle: 15 start/stop fault scenarios, reentrant cancellation, idempotence and physical descriptor replay passed\n";
}
