#pragma once
#include "archer/register_mailbox.hpp"
namespace archer {
struct ChipIdentity { unsigned revision, analog_revision, analog_id; };
namespace detail {
inline unsigned read_xtal_profile_byte(RegisterBus& bus, unsigned offset) {
    if(offset!=0x41 && offset!=0xfd && offset!=0xfe && offset!=0x2d && offset!=0x2e &&
       offset!=0x80 && offset!=0x81 && offset!=0x90 && offset!=0xa1 && offset!=0xe0 && offset!=0xe1)
        throw std::invalid_argument("Unsupported XTAL-SI read profile offset");
    const auto initial=bus.read(0x270,4);
    if(initial==0xeaeaeaea || initial==0xdeadbeef || (initial&(1u<<31)))
        throw std::runtime_error("XTAL-SI inaccessible or busy; no command sent");
    bus.write(0x270,4,0x81000000u|offset); // BE chip ID 0, normal READ, zero bitmask.
    for(unsigned attempt=0;attempt<6;++attempt) {
        const auto status=bus.read(0x270,4);
        if(status==0xeaeaeaea || status==0xdeadbeef)
            throw std::runtime_error("XTAL-SI became inaccessible");
        if(!(status&(1u<<31))) {
            if((status&255)!=offset || ((status>>24)&3)!=1 || ((status>>28)&7)!=0)
                throw std::runtime_error("XTAL-SI reply selector mismatch");
            return (status>>8)&255;
        }
        bus.pause();
    }
    throw std::runtime_error("XTAL-SI read timeout; no retry or reset");
}
} // namespace detail
inline unsigned read_xtal_identity_byte(RegisterBus& bus,unsigned offset) {
    if(offset!=0x41&&offset!=0xfd&&offset!=0xfe)throw std::invalid_argument("Only identity XTAL-SI reads supported");
    return detail::read_xtal_profile_byte(bus,offset);
}
inline unsigned read_xtal_power_byte(RegisterBus& bus,unsigned offset) {
    if(offset!=0x2d&&offset!=0x2e&&offset!=0x80&&offset!=0x81&&offset!=0x90&&offset!=0xa1&&offset!=0xe0&&offset!=0xe1)
        throw std::invalid_argument("Only fixed XTAL-SI power reads supported");
    return detail::read_xtal_profile_byte(bus,offset);
}
inline ChipIdentity read_chip_identity(RegisterBus& bus) {
    if((bus.read(0xfc,4)&255)!=0x71 || ((bus.read(0x3f0,4)>>16)&0x3ff)!=0x220)
        throw std::runtime_error("Unsupported hardware or inactive state");
    const auto revision=(bus.read(0xf0,4)>>12)&15;
    const auto analog_revision=read_xtal_identity_byte(bus,0x41)&15;
    const auto low=read_xtal_identity_byte(bus,0xfd);
    const auto high=read_xtal_identity_byte(bus,0xfe);
    if((low==255 && high==255) || (low==0 && high==0))
        throw std::runtime_error("Missing analog chip identifier");
    return {revision,analog_revision,low|(high<<8)};
}
}
