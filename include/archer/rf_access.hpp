#pragma once
#include <cstdint>
#include <stdexcept>
namespace archer {
constexpr std::uint32_t be_bb_base=0x20000;
inline std::uint32_t be_bb_register(std::uint32_t logical) {
    if(logical>0xfffff || logical%4) throw std::invalid_argument("Invalid aligned BB register");
    return be_bb_base+logical;
}
struct RfWritePlan {
    bool direct;
    std::uint32_t status_address, status_busy_mask, target_address, value, mask;
};
// RTL8922A write_rf_v2, full 20-bit writes only. No I/O or masked-write fallback.
inline RfWritePlan be_rf_write(unsigned path,std::uint32_t address,std::uint32_t value) {
    if(path>1 || (address & ~0x100ffu) || value>0xfffff)
        throw std::invalid_argument("Invalid RF path, selector/address or 20-bit value");
    if(address&0x10000) {
        const auto logical=(path==0?0xe000u:0xf000u)+((address&255)<<2);
        return {true,0,0,be_bb_register(logical),value,0xfffff};
    }
    return {false,be_bb_register(path==0?0x2c24:0x2d24),1u<<29,
            be_bb_register(path==0?0x2ae0:0x2be0),(value<<8)|(address&255),0xffffffff};
}
}
