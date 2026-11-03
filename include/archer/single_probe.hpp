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
// Explicit SSID-directed request for the user-owned 2G target. Broadcast DA/BSSID
// retain the proven BMC descriptor; SSID IE selects Especial. Tag2 and seq2
// distinguish this attempt from the historical wildcard request.
inline std::vector<std::uint8_t> special_channel9_probe_packet(){
    auto p=single_probe_packet();
    p.insert(p.begin()+90,{'E','s','p','e','c','i','a','l'});
    p[89]=8;p[86]=0x20;
    channel_put32(p.data()+8,0x00240028); // frame40, QSEL18.
    channel_put32(p.data()+12,0x2002); // Wi-Fi sequence2.
    channel_put32(p.data()+36,0x20000000); // Report tag2.
    return p;
}
inline std::vector<std::uint8_t> special_channel9_unicast_probe_packet(){
    auto p=special_channel9_probe_packet();
    const std::array<std::uint8_t,6> ap{0x50,0x92,0x6a,0x11,0x06,0x66};
    std::copy(ap.begin(),ap.end(),p.begin()+68); // DA.
    std::copy(ap.begin(),ap.end(),p.begin()+80); // BSSID.
    p[86]=0x30;
    channel_put32(p.data()+12,0x2003);
    channel_put32(p.data()+36,0x30000000); // Distinct report tag3.
    channel_put32(p.data()+48,0x88000000); // Source !is_bmc RTS_EN, HW_RTS_EN.
    return p;
}
// Instrumented contrast: keep the stage86 unicast frame but disable RTS
// explicitly; this is a diagnostic deviation from the source default.
inline std::vector<std::uint8_t> special_channel9_monitored_probe_packet(){
    auto p=special_channel9_unicast_probe_packet();p[86]=0x40;
    channel_put32(p.data()+12,0x2004);channel_put32(p.data()+36,0x40000000);
    channel_put32(p.data()+48,0x80000000);return p;
}
inline std::vector<std::uint8_t> special_channel9_ack_probe_packet(){
    auto p=special_channel9_monitored_probe_packet();p[86]=0x50;
    channel_put32(p.data()+12,0x2005);channel_put32(p.data()+36,0x50000000);return p;
}
inline std::vector<std::uint8_t> special_channel9_external_probe_packet(){
    auto p=special_channel9_ack_probe_packet();p[86]=0x60;
    channel_put32(p.data()+12,0x2006);channel_put32(p.data()+36,0x60000000);return p;
}
inline std::vector<std::uint8_t> special_channel9_phy_probe_packet(){
    auto p=special_channel9_external_probe_packet();p[86]=0x70;
    channel_put32(p.data()+12,0x2007);channel_put32(p.data()+36,0x70000000);return p;
}
struct ProbeTxReport {unsigned state,count;};
inline bool probe_report_terminal(const ProbeTxReport& r,unsigned limit){
    if(!limit||limit>63)throw std::invalid_argument("Invalid TX retry limit");
    // mac.c:5848-5853: failed attempts are progress until the requested count.
    return r.state==0||r.count==limit;
}
inline std::optional<ProbeTxReport> single_probe_report(const std::uint8_t* p,std::size_t size,unsigned expected_tag=1){
    if(expected_tag<1||expected_tag>15)throw std::invalid_argument("Invalid expected TX report tag");
    auto h=decode_c2h(p,size);
    if(h.category!=1||h.command_class!=9||h.function!=1)return {};
    if(size!=80)throw std::runtime_error("Unexpected RTL8922A TX report size");
    const auto w12=channel_u32(p+48),w14=channel_u32(p+56);
    if(((w12>>12)&15)!=expected_tag)return {};
    return ProbeTxReport{(w12>>8)&3,(w14>>10)&63};
}
}
