#pragma once
#include "archer/dle_init.hpp"
namespace archer {
// Explicit reset transition, not a live queue migration or a promise to retain packets.
// Caller owns the sole interface/project lock and has submitted no outstanding URBs.
inline void reset_to_usb2_scc(RegisterBus& bus,const std::function<void(unsigned)>& delay_us){
    auto get=[&](unsigned a){auto v=bus.read(a,4);if(v==0xdeadbeef||v==0xeaeaeaea)throw std::runtime_error("MAC reset register inaccessible");return v;};
    auto set=[&](unsigned a,unsigned mask,unsigned value){bus.write(a,4,(get(a)&~mask)|value);if((get(a)&mask)!=value)throw std::runtime_error("MAC reset control did not latch");};
    if((get(0x1e0)&0x3c000fff)!=0x0c000022||(get(0x84)&7)!=7||get(0x842c)!=0x7ff97ff9)
        throw std::runtime_error("Expected previously loaded research firmware/BB state");
    // Stop new DMA requests, assert WLAN/BB reset, then discard/reset the old DLE partition.
    set(0x7880,3,0);set(0xb010,0x7fff,0x7fff);set(0xb000,0xb0,0x80);
    set(0x84,7,0);set(0x842c,0xffffffff,0);
    set(0x88,2,0);set(0x88,0x800,0x800);
    bus.write(0x1e0,4,get(0x1e0)&0xc0000000);
    if((get(0x88)&0x802)!=0x800||(get(0x84)&7)||(get(0x7880)&3)||(get(0xb010)&0x7fff)!=0x7fff)
        throw std::runtime_error("Reset producer controls not asserted");
    set(0x8400,(1u<<26)|(1u<<23),0);
    delay_us(1000);
    initialize_usb2_scc_dle(bus,{true,true,true,true},[&]{delay_us(1);});
    set(0x8420,0x1000,0x1000);set(0xb000,0x7b0,0x430);set(0xb010,0x7fff,0);set(0x7880,3,3);
}
inline void enable_cmac0(RegisterBus& bus){
    auto set=[&](unsigned a,unsigned mask,unsigned value){auto v=bus.read(a,4);if(v==0xdeadbeef||v==0xeaeaeaea)throw std::runtime_error("CMAC control inaccessible");bus.write(a,4,(v&~mask)|value);if((bus.read(a,4)&mask)!=value)throw std::runtime_error("CMAC control did not latch");};
    set(0x24,0x1f000000,0x1f000000);set(0x84,0x1000000,0);set(0x84,0x10000,0x10000);
    set(0x10004,0x400001ff,0x400001ff);set(0x10000,0xf00001ff,0xf00001ff);
}
}
