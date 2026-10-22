#pragma once
#include "archer/channel_bus.hpp"
#include <array>
namespace archer {
// Fixed MAC0 diagnostic window, not the Linux scan lifecycle. No retune/TX.
inline void with_passive_rx_window(RegisterBus& bus,const std::function<void()>& receive) {
    if((channel_bus_read(bus,0xfc)&255)!=0x71 ||
       !(channel_bus_read(bus,0x10000)&0x40000000) ||
       (channel_bus_read(bus,0x10398)&0x3ffff))
        throw std::runtime_error("Passive RX requires RTL8922A CMAC0 with TX paused");
    constexpr std::array<unsigned,5> addr{0x10000,0x11424,0x1142c,0x11428,0x11420};
    constexpr std::array<unsigned,5> mask{0x80,0xffff,0xffff,0xffff,0x7bff};
    // Disable response-packet engine; reject control/data; admit beacon/probe response.
    constexpr std::array<unsigned,5> value{0,0,0,0x120,4};
    std::array<unsigned,5> saved{};
    for(unsigned i=0;i<addr.size();++i)saved[i]=channel_bus_read(bus,addr[i]);
    unsigned attempted=0;std::exception_ptr failure;
    try {
        for(unsigned i=0;i<addr.size();++i){attempted=i+1;channel_bus_write(bus,addr[i],mask[i],value[i]);}
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
