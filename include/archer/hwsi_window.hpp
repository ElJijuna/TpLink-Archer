#pragma once
#include "archer/register_mailbox.hpp"
#include <array>
namespace archer {
// Candidate from hash-bound RTL8922A BB table, RFE=1/CV=1. Bit 27 meaning unknown.
// A diagnostic hypothesis, not a replacement for complete BB initialization.
inline void with_hwsi_table_window(RegisterBus& bus,const std::function<void()>& inspect) {
    constexpr std::array<unsigned,2> controls{0x22adc,0x22bdc};
    std::array<unsigned,2> saved{};
    for(unsigned i=0;i<2;++i) {
        saved[i]=bus.read(controls[i],4);
        if((saved[i]&~4u)!=0x39000)throw std::runtime_error("Unexpected HWSI baseline; no table controls changed");
    }
    unsigned attempted=0;std::exception_ptr failure;
    try {
        for(unsigned i=0;i<2;++i) {
            attempted=i+1;
            bus.write(controls[i],4,0x08039000);
            if(bus.read(controls[i],4)!=0x08039000)throw std::runtime_error("HWSI table control did not latch");
        }
        bus.pause();inspect();
    }catch(...){failure=std::current_exception();}
    std::string restoration;
    while(attempted) {
        const auto i=--attempted;
        try {
            bus.write(controls[i],4,saved[i]);
            if(bus.read(controls[i],4)!=saved[i])throw std::runtime_error("HWSI control did not restore");
        }catch(const std::exception& e){restoration+=std::string(e.what())+"; ";}
    }
    if(!restoration.empty())throw std::runtime_error("HWSI table window cleanup failed: "+restoration);
    if(failure)std::rethrow_exception(failure);
}
// RTL8922A bb_postinit sets R_BBCLK bit 2 before RF initialization.
inline void with_bb_clock_window(RegisterBus& bus,const std::function<void()>& inspect) {
    constexpr unsigned clock=0x20000;
    const auto saved=bus.read(clock,4);
    if(saved!=0x1a)throw std::runtime_error("Unexpected BB clock baseline");
    std::exception_ptr failure;
    try {
        bus.write(clock,4,saved|4);
        if(bus.read(clock,4)!=(saved|4))throw std::runtime_error("BB clock did not latch");
        bus.pause();inspect();
    }catch(...){failure=std::current_exception();}
    bus.write(clock,4,saved);
    if(bus.read(clock,4)!=saved)throw std::runtime_error("BB clock did not restore");
    if(failure)std::rethrow_exception(failure);
}

}
