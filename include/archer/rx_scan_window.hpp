#pragma once
#include "archer/channel_bus.hpp"
#include <array>
namespace archer {
enum class RxAcceptance { broadcast, broadcast_bit_clear, sniffer, response_engine, all_management };
// Fixed MAC0 diagnostic window, not the Linux scan lifecycle. No retune or
// host TX submission. response_engine can permit automatic hardware responses.
inline void with_passive_rx_window(RegisterBus& bus,const std::function<void()>& receive,
    bool diagnostic_enable_rmac_channel=false,RxAcceptance acceptance=RxAcceptance::broadcast) {
    if(acceptance!=RxAcceptance::broadcast && acceptance!=RxAcceptance::broadcast_bit_clear && acceptance!=RxAcceptance::sniffer && acceptance!=RxAcceptance::response_engine && acceptance!=RxAcceptance::all_management)
        throw std::invalid_argument("Unknown RX acceptance diagnostic");
    if((channel_bus_read(bus,0xfc)&255)!=0x71 ||
       !(channel_bus_read(bus,0x10000)&0x40000000) ||
       (channel_bus_read(bus,0x10398)&0x3ffff))
        throw std::runtime_error("Passive RX requires RTL8922A CMAC0 with TX paused");
    constexpr std::array<unsigned,6> addr{0x10000,0x11424,0x1142c,0x11428,0x11420,0x11400};
    constexpr std::array<unsigned,6> mask{0x80,0xffff,0xffff,0xffff,0x7bff,1};
    // Normally disable response engine; explicit diagnostic can enable it.
    // Reject control/data; admit beacon/probe response (sniffer effect is empirical).
    const std::array<unsigned,6> value{acceptance==RxAcceptance::response_engine?0x80u:0u,0,0,
        acceptance==RxAcceptance::all_management?0xffffu:0x120u,
        acceptance==RxAcceptance::broadcast_bit_clear||acceptance==RxAcceptance::all_management?0u:acceptance==RxAcceptance::sniffer?5u:4u,1};
    const unsigned count=diagnostic_enable_rmac_channel?6:5;
    std::array<unsigned,6> saved{};
    for(unsigned i=0;i<count;++i)saved[i]=channel_bus_read(bus,addr[i]);
    unsigned attempted=0;std::exception_ptr failure;
    try {
        for(unsigned i=0;i<count;++i){attempted=i+1;channel_bus_write(bus,addr[i],mask[i],value[i]);}
        receive();
        if(channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("TX scheduler changed during passive RX");
    }catch(...){failure=std::current_exception();}
    std::string errors;
    while(attempted){const auto i=--attempted;
        try{channel_bus_write(bus,addr[i],mask[i],saved[i]&mask[i]);}
        catch(const std::exception& e){errors+=std::string(e.what())+"; ";}
    }
    if(!errors.empty())throw std::runtime_error("Passive RX restoration failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
