#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/channel_bus.hpp"
#include "archer/xtal_identity.hpp"
// Read-only verification except the standard WLAN IO window, restored on exit.
// Does not clear context, send H2C, reset firmware or reserve sequences.
int main(int argc,char** argv){
 if(argc!=2||std::string(argv[1])!="--paused-channel9"){std::cerr<<"Usage: cleanup-verify --paused-channel9\n";return 2;}
 try{
  archer::SequenceLedger owner("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
  archer::with_wlan_io_window(bus,[&]{
   if(archer::read_chip_identity(bus).analog_id!=0x6933||
      (archer::channel_bus_read(bus,0xfc)&255)!=0x71||
      (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022||
      (archer::channel_bus_read(bus,0x84)&0x300007)!=7||
      (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9||
      (archer::channel_bus_read(bus,0x10398)&0x3ffff)||
      (archer::channel_bus_read(bus,0x26b4c)&0x1fff)!=2452)
    throw std::runtime_error("Live paused channel9 firmware profile absent");
   archer::BeRfTransport rf(bus);
   if((rf.read_full(0,0x18)&0x31fff)!=9||(rf.read_full(0,0x10018)&0x31fff)!=9||
      (rf.read_full(1,0x10018)&0x31fff)!=9)throw std::runtime_error("Live RF9 absent");
   for(auto a:{0x28104u,0x28204u})if(archer::channel_bus_read(bus,a)&0x303)throw std::runtime_error("MCC0 absent");
   // Raw diagnostics only: IFS-CLM counters lack an armed interval/baseline.
   // R_BE_TXCNT is retry policy, not an accumulated transmission counter.
   for(auto a:{0x1082cu,0x108f0u,0x108f4u,0x20eccu,0x20ed0u})(void)bus.read(a,4);
   std::cout<<"{\"result\":\"paused_channel9_verified\",\"network_tx_paused\":true,\"h2c_submissions\":0,\"firmware_resets\":0,\"mcc_slot\":0}"<<std::endl;
  });return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
