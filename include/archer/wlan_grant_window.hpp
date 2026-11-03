#pragma once
#include "archer/channel_bus.hpp"
#include <ostream>
namespace archer {
// RTL8922A mac_be.c cfg_gnt_v2: PHY0 WL BB/TX/RX/power high, BT BB/TX/RX low.
// Preserve PHY1 BB grants, WLAN_ACT, reserved bits and every other register.
constexpr unsigned scc_grant_mask=0x3fc3f,scc_grant_wlan=0x17c1f;
inline void log_probe_grant_policy(RegisterBus& bus,const char* phase,std::ostream& out) {
 // RegisterBus reads emit their own JSONL. Complete the read before starting
 // another JSON object on the same stream.
 const auto value=channel_bus_read(bus,0xe348);
 out<<"{\"operation\":\"probe_grant_policy\",\"phase\":\""<<phase
    <<"\",\"value\":"<<value<<"}"<<std::endl;
}
inline void with_scc_wlan_grant_window(RegisterBus& bus,const std::function<void()>& action) {
 if(channel_bus_read(bus,0x10398)&0x3ffff ||
    channel_bus_read(bus,0x28104)&0x303 || channel_bus_read(bus,0x28204)&0x303)
  throw std::runtime_error("Grant contrast requires paused SCC/MCC0");
 const auto saved=channel_bus_read(bus,0xe348);
 if(saved!=0x01775774)throw std::runtime_error("Grant policy changed since measured blocked profile");
 std::exception_ptr error;
 try {
  channel_bus_write(bus,0xe348,scc_grant_mask,scc_grant_wlan);
  action();
  if(channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("TX must be paused before grant restoration");
 }catch(...){error=std::current_exception();}
 // Fail closed if the caller left TX enabled; do not change a live grant.
 if(channel_bus_read(bus,0x10398)&0x3ffff)
  throw std::runtime_error("TX remains active; grant restoration blocked");
 channel_bus_write(bus,0xe348,scc_grant_mask,saved&scc_grant_mask);
 if(error)std::rethrow_exception(error);
}
}
