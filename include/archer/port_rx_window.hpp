#pragma once
#include "archer/channel_bus.hpp"
namespace archer {
// PORT0 no-link RX fields/FUNC_EN experiment, NOT full mac_port_update/vif init.
inline void with_unlinked_port0_window(RegisterBus& bus,const std::function<void()>& receive){
    if((channel_bus_read(bus,0xfc)&255)!=0x71 ||
       !(channel_bus_read(bus,0x10000)&0x40000000) || (channel_bus_read(bus,0x10398)&0x3ffff))
        throw std::runtime_error("Port0 diagnostic requires paused RTL8922A CMAC0");
    const auto saved=channel_bus_read(bus,0x10400);
    // Require net type0, no beacon TX/force-TX or active port. Source no-link
    // clears BSSID fitting/TSF update, which may be set in a disabled baseline.
    if(saved&0x1e04)throw std::runtime_error("Port0 is not an inactive unlinked baseline");
    std::exception_ptr failure;
    try{channel_bus_write(bus,0x10400,0x1c,4);receive();}
    catch(...){failure=std::current_exception();}
    channel_bus_write(bus,0x10400,0x1c,saved&0x1c);
    if(failure)std::rethrow_exception(failure);
}
}
