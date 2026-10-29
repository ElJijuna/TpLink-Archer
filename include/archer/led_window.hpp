#pragma once
#include "archer/register_mailbox.hpp"

namespace archer {
// RTL8922A BE GPIO8. Static Windows evidence: research/STAGE69.md.
inline void with_gpio8_led(RegisterBus& bus, const std::function<void()>& observe) {
    const auto mux=bus.read(0x2d4,1), out=bus.read(0x61,1), dir=bus.read(0x62,2);
    auto masked=[&](unsigned reg,unsigned width,unsigned mask,unsigned value) {
        auto current=bus.read(reg,width);
        bus.write(reg,width,(current&~mask)|(value&mask));
        if((bus.read(reg,width)&mask)!=(value&mask))
            throw std::runtime_error("LED register readback mismatch");
    };
    auto restore=[&] {
        std::exception_ptr failure;
        auto attempt=[&](unsigned r,unsigned w,unsigned m,unsigned v) {
            try { masked(r,w,m,v); } catch(...) { if(!failure)failure=std::current_exception(); }
        };
        attempt(0x62,2,0x101,0);
        attempt(0x61,1,1,out);
        attempt(0x2d4,1,15,mux);
        attempt(0x62,2,0x101,dir);
        if(failure)std::rethrow_exception(failure);
    };
    try {
        masked(0x62,2,0x101,0); // Release pin before changing mux/output.
        masked(0x2d4,1,15,15);
        masked(0x61,1,1,0); // Active low candidate, corroborated by Linux LED8.
        masked(0x62,2,0x101,0x101);
        observe();
    } catch(...) {
        auto original=std::current_exception();
        try { restore(); } catch(...) {
            throw std::runtime_error("LED attempt failed and restoration could not be verified; reconnect USB");
        }
        std::rethrow_exception(original);
    }
    restore();
}
}
