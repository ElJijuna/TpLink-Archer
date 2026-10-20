#pragma once
#include "archer/register_mailbox.hpp"
namespace archer {
// Temporary RTL8922A BB reset-release diagnostic. Not full MAC/BB initialization.
inline void with_bb_diagnostic_window(RegisterBus& bus,const std::function<void()>& inspect) {
    auto get=[&](unsigned address,unsigned width) {
        const auto value=bus.read(address,width);
        if(value==0xeaeaeaea || value==0xdeadbeef)throw std::runtime_error("BB control inaccessible");
        return value;
    };
    if((get(0xfc,4)&255)!=0x71 || ((get(0xf0,4)>>12)&15)!=1 ||
       ((get(0x3f0,4)>>16)&0x3ff)!=0x220 || (get(0x74,4)&0x90000000)!=0x90000000)
        throw std::runtime_error("BB window hardware/power/WLAN prerequisites failed");
    const auto original_fen=get(0x84,1),original_dmac=get(0x842c,4);
    if((original_fen&0x33)!=0 || original_dmac!=0)
        throw std::runtime_error("BB window requires observed uninitialized, non-isolated state");
    bool attempted=false;
    std::exception_ptr failure;
    try {
        attempted=true;
        bus.write(0x84,1,original_fen|3);
        bus.write(0x842c,4,0x7ff97ff9);
        if((get(0x84,1)&3)!=3 || get(0x842c,4)!=0x7ff97ff9)
            throw std::runtime_error("BB reset-release controls did not latch");
        bus.pause();
        inspect();
    } catch(...) {failure=std::current_exception();}
    std::string restoration;
    if(attempted) {
        // Reassert BB reset before restoring its DMAC control fields.
        try {
            bus.write(0x84,1,(get(0x84,1)&~3u)|(original_fen&3));
            if((get(0x84,1)&3)!=(original_fen&3))throw std::runtime_error("BB reset bits did not restore");
        } catch(const std::exception& e){restoration+=std::string(e.what())+"; ";}
        try {
            bus.write(0x842c,4,original_dmac);
            if(get(0x842c,4)!=original_dmac)throw std::runtime_error("BB DMAC control did not restore");
        } catch(const std::exception& e){restoration+=std::string(e.what())+"; ";}
    }
    if(!restoration.empty())throw std::runtime_error("BB window restoration failed: "+restoration);
    if(failure)std::rethrow_exception(failure);
}
}
