#pragma once
#include "archer/firmware_channel.hpp"
namespace archer {
// Fixed broadcast probe request, channel1 CCK1, no encryption, one USB submission. Firmware retry semantics are not validated.
inline std::vector<std::uint8_t> single_probe_packet(){
    std::vector<std::uint8_t> p(96);
    channel_put32(p.data(),0x00486480); // WDINFO, DMA8, header24/2, STF, WD_PAGE.
    channel_put32(p.data()+8,0x00240020); // QSEL18, MACID0, frame32.
    channel_put32(p.data()+12,0x2001); // SW_MLD source management default, Wi-Fi sequence1.
    channel_put32(p.data()+28,0x80000000); // Fixed rate CCK1/20MHz.
    channel_put32(p.data()+32,0x00410400); // Explicit limit field1, disable fallback; RF attempts not guaranteed.
    channel_put32(p.data()+36,0x10000000); // SW report tag1.
    channel_put32(p.data()+40,0x40000000); // Report requested.
    channel_put32(p.data()+48,0x80000000); // BMC: no RTS, HW_RTS control.
    const std::array<std::uint8_t,32> frame{0x40,0,0,0,
        255,255,255,255,255,255, 2,0,0,0x89,0x22,0xa1,
        255,255,255,255,255,255, 0x10,0, 0,0, 1,4,0x82,0x84,0x8b,0x96};
    std::copy(frame.begin(),frame.end(),p.begin()+64);return p;
}
struct ProbeTxReport {unsigned state,count;};
inline std::optional<ProbeTxReport> single_probe_report(const std::uint8_t* p,std::size_t size){
    auto h=decode_c2h(p,size);
    if(h.category!=1||h.command_class!=9||h.function!=1)return {};
    if(size!=80)throw std::runtime_error("Unexpected RTL8922A TX report size");
    const auto w12=channel_u32(p+48),w14=channel_u32(p+56);
    if(((w12>>12)&15)!=1)return {};
    return ProbeTxReport{(w12>>8)&3,(w14>>10)&63};
}
}
