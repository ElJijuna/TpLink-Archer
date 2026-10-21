#pragma once
#include "archer/register_mailbox.hpp"
#include <vector>
namespace archer {
struct HfcUsb2SccProfile {
    std::array<unsigned,12> minimum{}, maximum{};
    unsigned group0=0, group1=0, public_maximum=0, write_pointer_threshold=0;
    unsigned channel_prealloc=0, h2c_prealloc=0, wp07_prealloc=0, wp811_prealloc=0;
    std::array<unsigned,4> full_conditions{}; // channel, H2C, WP07, WP811
    unsigned mode=0, dma_channel_skip_mask=0;
};
inline HfcUsb2SccProfile usb2_scc_hfc_profile() {
    HfcUsb2SccProfile p;
    for(unsigned ch=0;ch<12;ch+=2){p.minimum[ch]=54;p.maximum[ch]=222;}
    p.group0=p.public_maximum=492;p.channel_prealloc=18;p.h2c_prealloc=32;
    p.wp07_prealloc=p.wp811_prealloc=148;p.full_conditions={1,1,0,1};p.mode=1;
    return p;
}
inline void validate_usb2_scc_hfc_profile(const HfcUsb2SccProfile& p) {
    const auto expected=usb2_scc_hfc_profile();
    if(p.minimum!=expected.minimum || p.maximum!=expected.maximum || p.group0!=492 ||
       p.group1 || p.public_maximum!=492 || p.write_pointer_threshold ||
       p.channel_prealloc!=18 || p.h2c_prealloc!=32 || p.wp07_prealloc!=148 ||
       p.wp811_prealloc!=148 || p.full_conditions!=expected.full_conditions ||
       p.mode!=1 || p.dma_channel_skip_mask)
        throw std::runtime_error("HFC profile does not match pinned RTL8922A USB2 SCC");
}
enum class HfcInitKind { masked_write, delay_us };
struct HfcInitOp { HfcInitKind kind; unsigned address,mask,value; unsigned verify_mask=0xffffffff; };
inline std::vector<HfcInitOp> usb2_scc_hfc_plan(const HfcUsb2SccProfile& p) {
    validate_usb2_scc_hfc_profile(p);
    using K=HfcInitKind;
    std::vector<HfcInitOp> plan{{K::masked_write,0xb700,9,0}};
    for(unsigned ch=0;ch<12;++ch)
        plan.push_back({K::masked_write,0xb718+4*ch,0xffffffff,p.minimum[ch]|(p.maximum[ch]<<16)});
    plan.push_back({K::masked_write,0xb790,0xffffffff,492});
    plan.push_back({K::masked_write,0xb7a8,0xffffffff,0});
    plan.push_back({K::masked_write,0xb704,0xffffffff,0x200012});
    plan.push_back({K::masked_write,0xb794,0xffffffff,492});
    plan.push_back({K::masked_write,0xb7a4,0xffffffff,0x940094});
    // Linux reads CH12 full condition back into host state without requiring
    // the requested threshold to latch. Stage32 observed 0x512 -> 0x112.
    plan.push_back({K::masked_write,0xb700,0xff6,0x512,0xfffff3ff});
    plan.push_back({K::masked_write,0xb700,9,9});
    plan.push_back({K::delay_us,0,0,10});
    return plan;
}
struct HfcReadback {
    std::array<unsigned,12> channel_available{},channel_used{};
    unsigned group0_used=0,group1_used=0,group0_available=0,group1_available=0;
    unsigned public_available=0,write_pointer_available=0;
    unsigned control=0,channel_page_control=0,public_page_control1=0,public_page_control2=0;
    unsigned wp_page_control1=0,wp_page_control2=0;
    unsigned h2c_full_condition_requested=1,h2c_full_condition_effective=0;
    unsigned control_profile_difference=0;
};
inline HfcReadback initialize_usb2_scc_hfc(RegisterBus& bus,const HfcUsb2SccProfile& profile,
    bool exclusive_initialization_owner,bool usb2_scc_dle_ready,
    const std::function<void(unsigned)>& delay_us) {
    const auto plan=usb2_scc_hfc_plan(profile);
    if(!exclusive_initialization_owner || !usb2_scc_dle_ready || !delay_us)
        throw std::runtime_error("HFC requires exclusive lifecycle, initialized SCC DLE and timed delay");
    auto read=[&](unsigned address){
        const auto value=bus.read(address,4);
        if(value==0xeaeaeaea || value==0xdeadbeef)throw std::runtime_error("HFC register inaccessible");
        return value;
    };
    if((read(0xfc)&255)!=0x71 || ((read(0xf0)>>12)&15)!=1 ||
       (read(0x8400)&0x60000000)!=0x60000000 || (read(0x8d00)&3)!=3 || (read(0x9100)&3)!=3)
        throw std::runtime_error("HFC hardware/MAC/DLE prerequisites failed before writes");
    auto update=[&](unsigned address,unsigned mask,unsigned value,unsigned verify_mask=0xffffffff){
        bus.write(address,4,mask==0xffffffff?value:(read(address)&~mask)|value);
        if((read(address)&mask&verify_mask)!=(value&verify_mask))throw std::runtime_error("HFC configuration did not latch");
    };
    try {
        for(const auto& op:plan){
            if(op.kind==HfcInitKind::delay_us)delay_us(op.value);
            else update(op.address,op.mask,op.value,op.verify_mask);
        }
        HfcReadback info;
        for(unsigned ch=0;ch<12;++ch){
            const auto value=read(0xb750+4*ch);
            // Common source hfc_upd_ch_info deliberately uses AX 12-bit AVAL.
            info.channel_available[ch]=(value>>16)&0xfff;
            info.channel_used[ch]=value&0x1fff;
        }
        const auto used=read(0xb79c), available=read(0xb78c);
        info.group0_used=used&0x1fff;info.group1_used=(used>>16)&0x1fff;
        info.group0_available=available&0x1fff;info.group1_available=(available>>16)&0x1fff;
        info.public_available=read(0xb7a0)&0x1fff;
        info.write_pointer_available=(read(0xb7ac)>>16)&0x1fff;
        info.control=read(0xb700);info.channel_page_control=read(0xb704);
        info.public_page_control1=read(0xb790);info.public_page_control2=read(0xb794);
        info.wp_page_control1=read(0xb7a4);info.wp_page_control2=read(0xb7a8);
        info.h2c_full_condition_effective=(info.control>>10)&3;
        info.control_profile_difference=(info.control^0x51b)&0xfff;
        if((info.control&0x3ff)!=0x11b || (info.channel_page_control&0x3f003f)!=0x200012 ||
           (info.public_page_control1&0x1fff1fff)!=492 || (info.public_page_control2&0x1fff)!=492 ||
           (info.wp_page_control1&0x1ff01ff)!=0x940094 || (info.wp_page_control2&0x1fff))
            throw std::runtime_error("HFC final profile readback mismatch");
        if(info.group0_used+info.group1_used+info.public_available!=492)
            throw std::runtime_error("HFC public page accounting invariant failed");
        return info;
    } catch(...) {
        const auto failure=std::current_exception();
        try{update(0xb700,9,0);}
        catch(...){throw std::runtime_error("HFC failed and disable could not be verified; reset required");}
        std::rethrow_exception(failure);
    }
}
}
