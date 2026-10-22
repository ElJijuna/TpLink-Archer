#pragma once
#include "archer/channel_bus.hpp"
namespace archer {
// cfg_phy_rpt_be(true): PHY_RPT_SIZE_8=1, HDR_CNV_SIZE_0=0, PHYRPT_EN=1.
inline void with_phy_report_window(RegisterBus& bus,const std::function<void()>& receive){
    if((channel_bus_read(bus,0x10398)&0x3ffff) || !(channel_bus_read(bus,0x10000)&0x40000000))
        throw std::runtime_error("PHY report window requires paused CMAC0");
    const unsigned rcr=channel_bus_read(bus,0x11400),info=channel_bus_read(bus,0x11470);
    unsigned attempted=0;std::exception_ptr failure;
    try{
        attempted=1;channel_bus_write(bus,0x11400,0xf0,0x10);
        attempted=2;channel_bus_write(bus,0x11470,1,1);
        receive();
    }catch(...){failure=std::current_exception();}
    std::string errors;
    if(attempted>=2)try{channel_bus_write(bus,0x11470,1,info&1);}catch(const std::exception& e){errors+=e.what();}
    if(attempted>=1)try{channel_bus_write(bus,0x11400,0xf0,rcr&0xf0);}catch(const std::exception& e){errors+=e.what();}
    if(!errors.empty())throw std::runtime_error("PHY report cleanup failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
