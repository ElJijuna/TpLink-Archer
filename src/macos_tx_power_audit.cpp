#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/channel_bus.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/trx_path.hpp"
#include "archer/phy_tx_counters.hpp"
#include "channel_payload.hpp"
#include <map>
// Snapshot only: standard WLAN access-window writes are restored. No RFK,
// scheduler resume, firmware reset, H2C submission or network-frame submission.
int main(int argc,char** argv){
 if(argc!=2||std::string(argv[1])!="--paused-channel9"){
  std::cerr<<"Usage: tx-power-audit --paused-channel9\n";return 2;
 }
 try{
  archer::SequenceLedger owner("artifacts/37ad-0104-sequences.bin");
  MacRegisterBus bus;
  archer::with_wlan_io_window(bus,[&]{
   if(archer::read_chip_identity(bus).analog_id!=0x6933||
      (archer::channel_bus_read(bus,0xfc)&255)!=0x71||
      (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022||
      (archer::channel_bus_read(bus,0x84)&0x300007)!=7||
      (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9||
      (archer::channel_bus_read(bus,0x10398)&0x3ffff)||
      (archer::channel_bus_read(bus,0x26b4c)&0x1fff)!=2452)
    throw std::runtime_error("Paused bound channel9 profile absent");
   for(auto a:{0x28104u,0x28204u})
    if(archer::channel_bus_read(bus,a)&0x303)throw std::runtime_error("MCC0 absent");
   struct Expected {unsigned mask=0,value=0;};
   std::map<unsigned,Expected> expected;
   auto merge=[&](unsigned address,unsigned mask,unsigned value){
    auto& e=expected[address];e.value=(e.value&~mask)|(value&mask);e.mask|=mask;
   };
   for(const auto& op:archer::rtl8922a_scc_ab_trx_path_plan({true}))merge(op.address,op.mask,op.value);
   for(const auto& op:trusted_channel9_world_power)merge(op.address,op.mask,op.value);
   unsigned differences=0;
   for(const auto& [address,e]:expected){
    const auto raw=archer::channel_bus_read(bus,address);
    const bool same=(raw&e.mask)==e.value;
    differences+=!same;
    std::cout<<"{\"operation\":\"tx_power_snapshot\",\"address\":"<<address
      <<",\"mask\":"<<e.mask<<",\"expected_host_plan\":"<<e.value<<",\"observed\":"<<raw
      <<",\"matches_host_plan\":"<<(same?"true":"false")<<"}"<<std::endl;
   }
   // Fixed diagnostic registers selected from RTL8922A source. Counters have
   // no fresh baseline and must not be attributed to any previous network TX.
   // R_BE_GNT_SW_CTRL is the RTL8922A v2 grant policy register. Read raw
   // policy/BT coexistence controls without forcing either grant or RF path.
   for(auto address:{0x20980u,0x2e610u,0x2e710u,0x22e00u,0x22e04u,
                     0xe348u,0xe300u,0x1087cu,0x20c6cu}){
    const auto raw=archer::channel_bus_read(bus,address);
    std::cout<<"{\"operation\":\"tx_gate_raw_snapshot\",\"address\":"<<address
      <<",\"observed\":"<<raw<<",\"attributed_to_network_tx\":false}"<<std::endl;
   }
   for(unsigned sample=0;sample<3;++sample){
    const auto phy=archer::read_phy_tx_snapshot(bus);const auto counts=phy.counts();
    std::cout<<"{\"operation\":\"passive_phy_tx_counters\",\"sample\":"<<sample
      <<",\"controls_enabled\":"<<(phy.enabled()?"true":"false")<<",\"counts\":[";
    for(unsigned i=0;i<4;++i){if(i)std::cout<<',';std::cout<<counts[i];}
    std::cout<<"],\"attributed_to_network_tx\":false}"<<std::endl;
    if(sample!=2)std::this_thread::sleep_for(std::chrono::milliseconds(150));
   }
   std::cout<<"{\"result\":\"tx_power_snapshot_complete\",\"registers\":"<<expected.size()
     <<",\"differences_from_host_plan\":"<<differences
     <<",\"network_tx_paused\":true,\"h2c_submissions\":0,\"firmware_resets\":0}"<<std::endl;
  });return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
