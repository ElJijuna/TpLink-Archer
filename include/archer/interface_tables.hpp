#pragma once
#include "archer/firmware_channel.hpp"
#include <array>
namespace archer {
// Firmware0.35.41 selects MACID_PAUSE_SLEEP (>=0.34.11), not old pause32.
inline std::vector<std::uint8_t> encode_station_macid_pause_sleep(unsigned sequence,
    unsigned mac_id,bool pause){
    if(mac_id>127)throw std::invalid_argument("MACID pause exceeds chip allocation range");
    std::vector<std::uint8_t> payload(256);
    const unsigned group=mac_id/32,bit=1u<<(mac_id%32);
    channel_put32(payload.data()+4*(4+group),bit);
    channel_put32(payload.data()+4*(12+group),bit);
    if(pause){
        channel_put32(payload.data()+4*group,bit);
        channel_put32(payload.data()+4*(8+group),bit);
    }
    return encode_h2c(1,9,0x28,sequence,payload,true,false);
}
// RTL8922A defaults, fw.c default_cmac_tbl_g7/default_dmac_tbl_v2. No I/O here.
// Both payloads: c0, w0..15, m0..15 =132 bytes. Not direct AX table I/O.
inline std::vector<std::uint8_t> encode_default_station_cmac_g7(unsigned sequence,
    unsigned mac_id,bool tx_report_enabled){
    if(mac_id>127)throw std::invalid_argument("CMAC G7 MACID exceeds 7 bits");
    const std::array<unsigned,16> values{4u|(tx_report_enabled?0x200000u:0u),
        0x400a0004,0,0,0xffff0000,0x2aa,0xb000,0xb8109,0,0,0,0,0,0,0,0};
    const std::array<unsigned,16> masks{0xfff7ffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffdfff,0xffff7fff,0x80ffffff,0xefffefff,0xffff,0,0,0,0,0,0xffffffff,0x0fffffff};
    std::vector<std::uint8_t> payload(132);
    channel_put32(payload.data(),mac_id|0x80);
    for(unsigned i=0;i<16;++i){
        channel_put32(payload.data()+4*(1+i),values[i]);
        channel_put32(payload.data()+4*(17+i),masks[i]);
    }
    return encode_h2c(1,5,0x11,sequence,payload,false,true);
}
inline std::vector<std::uint8_t> encode_default_station_dmac_v2(unsigned sequence,unsigned mac_id){
    if(mac_id>127)throw std::invalid_argument("DMAC V2 MACID exceeds 7 bits");
    const std::array<unsigned,16> masks{0xffffffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffffe0,0xffffff0f,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffffff,0xffffffff,0xffff,0,0,0};
    std::vector<std::uint8_t> payload(132);
    channel_put32(payload.data(),mac_id|0x80);
    for(unsigned i=0;i<16;++i)channel_put32(payload.data()+4*(17+i),masks[i]);
    // Source requests neither receive nor done ACK for this command.
    return encode_h2c(1,5,0x0c,sequence,payload,false,false);
}
}
