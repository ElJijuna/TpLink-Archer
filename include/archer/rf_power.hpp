#pragma once
#include "archer/xtal_identity.hpp"
#include <vector>
namespace archer {
// rtw8922a_pwr_on_func: prerequisites for the converter calibration clocks.
// Leaves the documented power-on state active; never toggles MCU resets.
inline void enable_adda_power(RegisterBus& bus) {
    if((bus.read(0xfc,4)&255)!=0x71 || ((bus.read(0xf0,4)>>12)&15)!=1 ||
       (bus.read(0x74,4)&0x90000000)!=0x90000000)
        throw std::runtime_error("ADDA power requires active RTL8922A CV1 IO");
    auto set=[&](unsigned a,unsigned width,unsigned mask,unsigned value){
        const auto old=bus.read(a,width);
        if(old==0xdeadbeef || old==0xeaeaeaea)throw std::runtime_error("ADDA power control inaccessible");
        bus.write(a,width,(old&~mask)|value);
        if((bus.read(a,width)&mask)!=value)throw std::runtime_error("ADDA power control did not latch");
    };
    set(0x244,4,1u<<28,0);
    set(0x16,1,0xc,0xc);
    set(0x84,4,0x300000,0);
}
inline void write_xtal_power_masked(RegisterBus& bus,unsigned offset,unsigned value,unsigned mask) {
    unsigned allowed=offset==0xe0?3:offset==0xe1?0xf8:offset==0xd1?0x60:offset==0x16?0x38:offset==0x90||offset==0x80||offset==0x81?255:
        offset==0x2d||offset==0x2e?0x40:offset==0xa1?2:0;
    if(!mask||(mask&~allowed)||value>255)throw std::invalid_argument("Unsupported XTAL power write");
    const auto before=read_xtal_power_byte(bus,offset);
    const auto expected=(before&~mask)|(value&mask);
    bus.write(0x270,4,0x80000000u|(mask<<16)|(value<<8)|offset);
    bool done=false;
    for(unsigned i=0;i<6;++i){
        auto s=bus.read(0x270,4);
        if(s==0xdeadbeef||s==0xeaeaeaea)throw std::runtime_error("XTAL write inaccessible");
        if(!(s>>31)){if((s&255)!=offset||((s>>24)&3))throw std::runtime_error("XTAL write reply mismatch");done=true;break;}
        bus.pause();
    }
    if(!done)throw std::runtime_error("XTAL power write timeout");
    if(read_xtal_power_byte(bus,offset)!=expected)throw std::runtime_error("XTAL power field did not latch");
}
inline void initialize_rfk_pll(RegisterBus& bus) {
    const auto id=read_chip_identity(bus);
    if(id.revision!=1 || id.analog_id!=0x6933 || (bus.read(0x74,4)&0x90000000)!=0x90000000)
        throw std::runtime_error("RFK PLL requires bound CV1 analog identity and WLAN IO");
    // rtw8922a_rfk_pll_init, called by phy_dm_init -> rfk_hw_init.
    write_xtal_power_masked(bus,0xe1,0xf8,0xf8);
    write_xtal_power_masked(bus,0xd1,0,0x60);
    write_xtal_power_masked(bus,0x16,0x38,0x38);
}
// Fixed power-on subsequence from RTL8922A, then restoration in reverse order.
// Diagnostic only: no CPU reset, firmware download, channel selection or RF table.
inline void with_rf_power_window(RegisterBus& bus,const std::function<void()>& inspect,bool keep_enabled_on_success=false) {
    if((bus.read(0xfc,4)&255)!=0x71||((bus.read(0xf0,4)>>12)&15)!=1||
       ((bus.read(0x3f0,4)>>16)&0x3ff)!=0x220||(bus.read(0x74,4)&0x90000000)!=0x90000000)
        throw std::runtime_error("RF power prerequisites missing");
    const unsigned offsets[]={0xe0,0x90,0x80,0x81,0x2d,0x2e,0xe1,0xa1};
    const unsigned expected[]={0,0x90,0xc6,0xc6,0x30,0x30,0x52,0};
    for(unsigned i=0;i<8;++i)if(read_xtal_power_byte(bus,offsets[i])!=expected[i])throw std::runtime_error("Unexpected RF power baseline");
    std::vector<std::function<void()>> undo;
    auto mmio=[&](unsigned a,unsigned width,unsigned mask,unsigned value){
        auto old=bus.read(a,width);if(old==0xdeadbeef||old==0xeaeaeaea)throw std::runtime_error("Power control inaccessible");
        undo.emplace_back([&,a,width,mask,old]{auto v=bus.read(a,width);if(v==0xdeadbeef||v==0xeaeaeaea)throw std::runtime_error("Power restoration inaccessible");bus.write(a,width,(v&~mask)|(old&mask));if((bus.read(a,width)&mask)!=(old&mask))throw std::runtime_error("Power restoration mismatch");});
        bus.write(a,width,(old&~mask)|(value&mask));
        if((bus.read(a,width)&mask)!=(value&mask))throw std::runtime_error("Power control did not latch");
    };
    auto xtal=[&](unsigned a,unsigned value,unsigned mask){
        auto old=read_xtal_power_byte(bus,a);
        undo.emplace_back([&,a,mask,old]{write_xtal_power_masked(bus,a,old,mask);});
        write_xtal_power_masked(bus,a,value,mask);
    };
    std::exception_ptr failure;
    try{
        xtal(0xe0,2,2);xtal(0xe0,1,1);
        mmio(0x18,4,0x40,0x40);xtal(0x90,0x40,0x40);
        mmio(0x18,4,0x20,0x20);xtal(0x90,0x20,0x20);
        xtal(0x90,4,4);xtal(0x90,8,8);xtal(0x90,0,0x10);
        xtal(0x80,0xeb,255);xtal(0x81,0xeb,255);
        xtal(0x90,1,1);xtal(0x90,2,2);xtal(0x90,0,0x80);
        xtal(0x2d,0,0x40);xtal(0x2e,0,0x40);xtal(0xe1,0x40,0x60);xtal(0xa1,0,2);
        mmio(0xcc,1,4,4);mmio(0,2,0x100,0x100);mmio(0,2,0x8000,0);bus.pause();mmio(0,2,0x4000,0);mmio(0xcc,1,4,0);
        inspect();
    }catch(...){failure=std::current_exception();}
    if(!failure&&keep_enabled_on_success)return;
    std::string errors;
    for(auto i=undo.rbegin();i!=undo.rend();++i){try{(*i)();}catch(const std::exception& e){errors+=std::string(e.what())+"; ";}}
    if(!errors.empty())throw std::runtime_error("RF power restoration failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
