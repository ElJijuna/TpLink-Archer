#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
namespace archer {
inline std::array<std::uint8_t,31> installer_eject_cbw(bool eject) {
    std::array<std::uint8_t,31> out{};
    out[0]=0x55;out[1]=0x53;out[2]=0x42;out[3]=0x43;
    out[4]=eject?0x97:0x87;out[5]=0x65;out[6]=0x43;out[7]=0x21;
    out[14]=6;out[15]=eject?0x1b:0x1e;out[19]=eject?2:0;
    return out; // LUN 0, zero data length, allow removal then STOP/LOEJ.
}
inline unsigned installer_csw(const std::array<std::uint8_t,13>& csw,bool eject) {
    const auto command=installer_eject_cbw(eject);
    if(csw[0]!=0x55||csw[1]!=0x53||csw[2]!=0x42||csw[3]!=0x53)
        throw std::runtime_error("Invalid storage CSW signature");
    for(unsigned i=4;i<8;++i)if(csw[i]!=command[i])throw std::runtime_error("Storage CSW tag mismatch");
    for(unsigned i=8;i<12;++i)if(csw[i])throw std::runtime_error("Unexpected storage residue");
    if(csw[12]>1)throw std::runtime_error("Storage phase error");
    return csw[12];
}
inline unsigned installer_csw_packet(const std::uint8_t* bytes,std::size_t size,bool eject) {
    if(!bytes || size!=13)throw std::runtime_error("Invalid storage CSW transfer size");
    // BOT section 5.3.3 requires an exactly 13-byte CSW.
    std::array<std::uint8_t,13> csw{};
    std::copy(bytes,bytes+13,csw.begin());
    return installer_csw(csw,eject);
}

}
