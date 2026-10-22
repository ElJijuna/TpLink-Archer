#pragma once
#include "archer/firmware_channel.hpp"
namespace archer {
// Hardware configured FCS policy is independent of the descriptor CRC error bit.
// Unknown remains unknown: callers must not silently strip four payload bytes.
enum class RxFcsPolicy { unknown, appended, stripped };
struct BeRxMetadata {
    RxPacket packet{};
    unsigned descriptor_bytes=0, metadata_bytes=0;
    unsigned frame_type=0, mac_id=0, security_type=0;
    unsigned bandwidth_code=0, rate_code=0, gi_ltf_code=0, ppdu_type=0, ppdu_count=0;
    std::uint32_t free_run_counter=0;
    bool crc_error=false, icv_error=false, hardware_decrypted=false, software_decrypted=false;
    bool ampdu=false, addr_cam_valid=false, phy1=false;
    // Only long RXD contains HDR_CNV; short RXD cannot establish framing.
    std::optional<bool> header_converted;
    std::optional<unsigned> header_offset, wireless_header_iv_bytes;
    RxFcsPolicy fcs_policy=RxFcsPolicy::unknown;
    bool normal_80211_candidate(bool verified_short_profile_no_conversion=false)const {
        return packet.packet_type==0 && !crc_error && !icv_error &&
            (header_converted.has_value() ? !*header_converted : verified_short_profile_no_conversion);
    }
    std::size_t payload_without_fcs_bytes()const {
        if(fcs_policy==RxFcsPolicy::unknown)
            throw std::invalid_argument("RX FCS presence requires actual MAC configuration evidence");
        if(fcs_policy==RxFcsPolicy::appended){
            if(packet.payload_bytes<4)throw std::invalid_argument("RX payload shorter than appended FCS");
            return packet.payload_bytes-4;
        }
        return packet.payload_bytes;
    }
};
inline std::vector<BeRxMetadata> decode_be_rx_metadata(const std::vector<std::uint8_t>& bytes,
    RxFcsPolicy configured_fcs=RxFcsPolicy::unknown) {
    // Reuse aggregate bounds/layout validation; never accept caller-forged offsets.
    const auto packets=decode_usb_rx_be(bytes);
    std::vector<BeRxMetadata> result;
    for(const auto& p:packets){
        const auto* d=bytes.data()+p.descriptor_offset;
        const auto w0=channel_u32(d),w2=channel_u32(d+8),w3=channel_u32(d+12),w4=channel_u32(d+16);
        BeRxMetadata m;m.packet=p;m.descriptor_bytes=(w0>>31)?40:24;
        m.metadata_bytes=static_cast<unsigned>(p.payload_offset-p.descriptor_offset-m.descriptor_bytes);
        m.phy1=(w0>>30)&1;m.frame_type=(w2>>10)&3;m.mac_id=w2&255;m.addr_cam_valid=(w2>>14)&1;
        m.security_type=w3&15;m.crc_error=w3&(1u<<6);m.icv_error=w3&(1u<<7);
        m.hardware_decrypted=w3&(1u<<8);m.software_decrypted=w3&(1u<<9);m.ampdu=w3&(1u<<11);
        m.bandwidth_code=(w4>>12)&7;m.rate_code=(w4>>20)&0xfff;m.gi_ltf_code=(w4>>16)&7;
        m.ppdu_type=w4&255;m.ppdu_count=(w4>>8)&7;m.free_run_counter=channel_u32(d+20);
        if(m.descriptor_bytes==40){
            m.header_converted=bool(channel_u32(d+28)&(1u<<16));
            const auto w9=channel_u32(d+36);m.header_offset=(w9>>16)&31;
            m.wireless_header_iv_bytes=(w9>>21)&63;
        }
        // FCS policy is meaningful only for normal packet payloads, not C2H.
        if(p.packet_type==0)m.fcs_policy=configured_fcs;
        result.push_back(m);
    }
    return result;
}
}
