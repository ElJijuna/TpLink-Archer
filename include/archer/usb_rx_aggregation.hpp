#pragma once
#include "archer/channel_bus.hpp"
namespace archer {
// usb.c rx_agg_cfg_v3 for RTL8922A: enabled, count255, time32*32us,
// buffer20KiB, RXAGG1=1f. Window restores full source-configured registers.
inline void with_usb_rx_aggregation(RegisterBus& bus,const std::function<void()>& receive){
    if((channel_bus_read(bus,0xfc)&255)!=0x71 ||
       (channel_bus_read(bus,0x10398)&0x3ffff) || (channel_bus_read(bus,0x7880)&7)!=7)
        throw std::runtime_error("USB aggregation requires paused RTL8922A and HCI enabled");
    const unsigned saved0=channel_bus_read(bus,0x6000),saved1=channel_bus_read(bus,0x6004);
    unsigned attempted=0;std::exception_ptr failure;
    try{
        attempted=1;channel_bus_write(bus,0x6000,0xffffffff,0x80ff2014);
        attempted=2;channel_bus_write(bus,0x6004,0xffffffff,0x1f);
        receive();
    }catch(...){failure=std::current_exception();}
    std::string errors;
    if(attempted>=2)try{channel_bus_write(bus,0x6004,0xffffffff,saved1);}catch(const std::exception& e){errors+=e.what();}
    if(attempted>=1)try{channel_bus_write(bus,0x6000,0xffffffff,saved0);}catch(const std::exception& e){errors+=e.what();}
    if(!errors.empty())throw std::runtime_error("USB aggregation cleanup failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
