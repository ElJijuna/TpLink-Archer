#pragma once
#include "archer/channel_bus.hpp"
namespace archer {
// RTL8922A PHY0 PMAC reset from phy.c; permanently discards old statistics.
// Baseline controls are required and restored on every path; no RF/BB reset.
inline void reset_rx_diagnostic_counters(RegisterBus& bus){
    const auto cck=channel_bus_read(bus,0x20540),all=channel_bus_read(bus,0x20730);
    if(!(cck&0x80) || (all&3)!=2 || (channel_bus_read(bus,0x10398)&0x3ffff))
        throw std::runtime_error("PMAC reset requires released counters and paused TX");
    std::exception_ptr failure;
    try{
        channel_bus_write(bus,0x20540,0x80,0);
        channel_bus_write(bus,0x20540,0x80,0x80);
        channel_bus_write(bus,0x20730,1,1);
        channel_bus_write(bus,0x20730,1,0);
    }catch(...){failure=std::current_exception();}
    std::string errors;
    for(const auto item:{std::pair<unsigned,unsigned>{0x20730,1},{0x20540,0x80}}){
        try{channel_bus_write(bus,item.first,item.second,item.first==0x20730?0:0x80);}
        catch(const std::exception& e){errors+=std::string(e.what())+"; ";}
    }
    if(!errors.empty())throw std::runtime_error("PMAC reset cleanup failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
