#pragma once
#include "archer/channel_bus.hpp"
#include <array>
namespace archer {
struct PhyTxSnapshot {
 unsigned cck_control,all_control,cck_txen_raw,cck_txon_raw,ofdm_raw;
 std::array<unsigned,4> counts()const {
  return {cck_txen_raw>>16,cck_txon_raw&0xffff,ofdm_raw>>16,ofdm_raw&0xffff};
 }
 bool enabled()const{return (cck_control&0x80)&&(all_control&3)==2;}
};
// RTL8922A pmac_regs: CCK TXEN upper16, CCK TXON lower16;
// OFDM TXEN upper16 and TXON lower16. All addresses are physical PHY0.
inline PhyTxSnapshot read_phy_tx_snapshot(RegisterBus& bus){
 return {channel_bus_read(bus,0x20540),channel_bus_read(bus,0x20730),
  channel_bus_read(bus,0x22e00),channel_bus_read(bus,0x22e04),channel_bus_read(bus,0x20e40)};
}
inline std::array<unsigned,4> phy_tx_modulo_delta(const PhyTxSnapshot& before,
                                               const PhyTxSnapshot& after){
 if(!before.enabled()||!after.enabled()||
    ((before.cck_control^after.cck_control)&0x80)||
    ((before.all_control^after.all_control)&3))
  throw std::runtime_error("PHY TX counters held, disabled or controls changed");
 const auto a=before.counts(),b=after.counts();std::array<unsigned,4> delta{};
 for(unsigned i=0;i<4;++i)delta[i]=(b[i]-a[i])&0xffff;
 return delta; // Modulo only: cannot distinguish wrap, saturation or an intervening reset.
}
}
