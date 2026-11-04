#include "archer/macos_register_bus.hpp"
#include "archer/workflow_lock.hpp"
// Inspect even while an epoch is fenced; no ledger reservation or fence mutation.
int main(int argc,char**){
 if(argc!=1)return 2;
 try{
  archer::WorkflowLock owner("artifacts/37ad-0104-sequences.bin.workflow");MacRegisterBus bus;
  archer::with_wlan_io_window(bus,[&]{
   const auto bb=bus.read(0x84,4),reset=bus.read(0x842c,4),fw=bus.read(0x1e0,4);
   std::cout<<"{\"result\":\"cold_profile_inspected\",\"bb_control\":"<<bb<<",\"bb_reset\":"<<reset<<",\"firmware_control\":"<<fw<<",\"cold_profile\":"<<(!(bb&0x33)&&reset==0?"true":"false")<<",\"h2c_submissions\":0,\"firmware_resets\":0}"<<std::endl;
  });return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
