#pragma once
#include "firmware_channel.hpp"
#include <array>

namespace archer {
// RTL8922A only. Deliberately bounded to PRE_NOTIFY V1 firmware support.
struct RfkFirmwareVersion {
    unsigned major, minor, sub, index;
};
inline void require_rfk_v1_firmware(RfkFirmwareVersion v) {
    if (v.major != 0 || v.minor != 35 || v.sub < 31 || v.sub >= 49 || v.index > 255)
        throw std::invalid_argument("RFK encoder supports RTL8922A 0.35.31.x through 0.35.48.x only");
}
struct RfkChannel {
    unsigned band; // rtw89_band: 0=2 GHz, 1=5 GHz, 2=6 GHz
    unsigned bandwidth; // rtw89_bandwidth: 0=20, 1=40, 2=80, 3=160, 4=320
    unsigned channel; // center channel, not primary-channel index
};
inline void validate_rfk_channel(const RfkChannel& c) {
    if (c.band > 2 || c.bandwidth > 4 || c.channel == 0 || c.channel > 233 ||
        (c.band == 0 && (c.channel > 14 || c.bandwidth > 1)) ||
        (c.band == 1 && c.bandwidth > 3))
        throw std::invalid_argument("Invalid RFK channel fields");
    // This is wire validation, not regulatory or channel-plan authorization.
}
inline void validate_rfk_phy(unsigned phy) {
    if (phy > 1) throw std::invalid_argument("RFK PHY must be 0 or 1");
}
struct RfkPreNotifyV1 {
    std::array<std::array<std::uint32_t, 3>, 2> channels{};
    std::array<std::array<std::uint32_t, 3>, 2> bands{};
    std::uint32_t mlo_mode{};
    std::array<unsigned, 2> table_indices{};
    unsigned phy{};
    bool mlo_1_1{};
};
inline std::vector<std::uint8_t> rfk_pre_notify_payload(RfkFirmwareVersion version,
                                                     const RfkPreNotifyV1& info) {
    require_rfk_v1_firmware(version);
    validate_rfk_phy(info.phy);
    if (info.table_indices[0] >= 3 || info.table_indices[1] >= 3)
        throw std::invalid_argument("RFK table index exceeds three tables");
    // 12 table words + MLO mode + four current table words + PHY + MLO 1+1.
    std::vector<std::uint8_t> payload(76);
    std::size_t offset = 0;
    auto put = [&](std::uint32_t word) { channel_put32(payload.data() + offset, word); offset += 4; };
    for (const auto& path : info.channels) for (auto ch : path) {
        if (ch > 233) throw std::invalid_argument("RFK table channel out of range");
        put(ch); // Zero entries are valid unused MCC slots.
    }
    for (const auto& path : info.bands) for (auto band : path) {
        if (band > 2) throw std::invalid_argument("RFK table band out of range");
        put(band);
    }
    put(info.mlo_mode);
    for (unsigned p = 0; p < 2; ++p) put(info.channels[p][info.table_indices[p]]);
    for (unsigned p = 0; p < 2; ++p) put(info.bands[p][info.table_indices[p]]);
    put(info.phy);
    put(info.mlo_1_1);
    return payload;
}
inline std::vector<std::uint8_t> rfk_dack_payload(RfkFirmwareVersion version, unsigned phy) {
    require_rfk_v1_firmware(version);
    validate_rfk_phy(phy);
    return {3, static_cast<std::uint8_t>(phy), 0};
}
inline std::vector<std::uint8_t> rfk_rxdck_payload(RfkFirmwareVersion version, unsigned phy,
        const RfkChannel& channel, bool debug = false, bool channel_calibration = false) {
    require_rfk_v1_firmware(version);
    validate_rfk_phy(phy);
    validate_rfk_channel(channel);
    const bool v0 = version.sub < 42; // __CFG_FW_FEAT(RTL8922A, lt, 0,35,42,0,...)
    std::vector<std::uint8_t> payload{static_cast<std::uint8_t>(v0 ? 8 : 9),
        static_cast<std::uint8_t>(phy), 0, 3, static_cast<std::uint8_t>(channel.band),
        static_cast<std::uint8_t>(channel.bandwidth), static_cast<std::uint8_t>(channel.channel),
        static_cast<std::uint8_t>(debug)};
    if (!v0) payload.push_back(channel_calibration);
    return payload;
}
inline void validate_rfk_cv(unsigned cv) {
    if (cv > 255) throw std::invalid_argument("RFK cut version exceeds wire width");
}
inline std::vector<std::uint8_t> rfk_txgapk_payload(RfkFirmwareVersion version, unsigned phy,
        const RfkChannel& channel, unsigned cv) {
    require_rfk_v1_firmware(version); validate_rfk_phy(phy); validate_rfk_channel(channel);
    validate_rfk_cv(cv);
    return {8, 2, static_cast<std::uint8_t>(phy), 3, static_cast<std::uint8_t>(channel.band),
        static_cast<std::uint8_t>(channel.bandwidth), static_cast<std::uint8_t>(channel.channel),
        static_cast<std::uint8_t>(cv)};
}
inline std::vector<std::uint8_t> rfk_iqk_payload(RfkFirmwareVersion version, unsigned phy,
        const RfkChannel& channel, unsigned cv, unsigned kpath) {
    require_rfk_v1_firmware(version); validate_rfk_phy(phy); validate_rfk_channel(channel);
    validate_rfk_cv(cv);
    // Caller resolves rtw89_phy_get_kpath from MLO/DBCC context. RTL8922A has A/B.
    if (kpath < 1 || kpath > 3) throw std::invalid_argument("Invalid RTL8922A IQK path bitmap");
    // IQK_V0 is only for versions <0.35.28.0, excluded by our supported range.
    return {8, 0, static_cast<std::uint8_t>(phy), static_cast<std::uint8_t>(kpath),
        static_cast<std::uint8_t>(channel.band), static_cast<std::uint8_t>(channel.bandwidth),
        static_cast<std::uint8_t>(channel.channel), static_cast<std::uint8_t>(cv)};
}
inline std::vector<std::uint8_t> rfk_dpk_payload(RfkFirmwareVersion version, unsigned phy,
        const RfkChannel& channel, bool debug = false) {
    require_rfk_v1_firmware(version); validate_rfk_phy(phy); validate_rfk_channel(channel);
    return {8, static_cast<std::uint8_t>(phy), 1, 3, static_cast<std::uint8_t>(channel.band),
        static_cast<std::uint8_t>(channel.bandwidth), static_cast<std::uint8_t>(channel.channel),
        static_cast<std::uint8_t>(debug)};
}
// TSSI NORMAL needs channel-specific efuse/trim DE values and firmware thermal tables.
// Those transformations are not ported; never substitute zero calibration data.
inline std::vector<std::uint8_t> rfk_tssi_normal_payload(RfkFirmwareVersion version,
        unsigned phy, const RfkChannel& channel, unsigned cv, unsigned rfe) {
    require_rfk_v1_firmware(version); validate_rfk_phy(phy); validate_rfk_channel(channel);
    validate_rfk_cv(cv);
    if(rfe > 255)throw std::invalid_argument("RFK RFE exceeds wire width");
    throw std::runtime_error("TSSI NORMAL unavailable: validated channel-specific efuse/trim DE and thermal tracking table preparation required");
}
enum class RfkCommand : unsigned { tssi = 0, iqk = 1, dpk = 3, txgapk = 4,
                                  dack = 5, rxdck = 6, pre_notify = 8 };
inline std::vector<std::uint8_t> encode_rfk_h2c(RfkCommand command, unsigned sequence,
                                             const std::vector<std::uint8_t>& payload) {
    if (command == RfkCommand::tssi)
        throw std::invalid_argument("TSSI encoding requires unimplemented host calibration data preparation");
    // No transport ACK requested by the pinned Linux RFK implementation.
    return encode_h2c(2, 0xb, static_cast<unsigned>(command), sequence, payload);
}
enum class RfkReportState : unsigned { start = 0, ok = 1, fail = 2, timeout = 3, command_error = 4 };
struct RfkReport {
    RfkReportState state;
    unsigned version;
    // Stage30 firmware 0.35.41.0 emits four bytes beyond the source prefix.
    // Preserve them without assigning command/sequence/PHY semantics.
    std::vector<std::uint8_t> opaque_extension;
};
inline std::optional<RfkReport> decode_rfk_report(const std::uint8_t* bytes, std::size_t size) {
    const auto h = decode_c2h(bytes, size);
    if (h.category != 2 || h.command_class != 9 || h.function != 0) return std::nullopt;
    if ((size != 10 && size != 14) || bytes[8] > 4)
        throw std::invalid_argument("Malformed RFK state report");
    return RfkReport{static_cast<RfkReportState>(bytes[8]), bytes[9],
                     std::vector<std::uint8_t>(bytes + 10, bytes + size)};
}
// RFK state C2H has no sequence, command, or PHY. Caller must serialize RFK,
// drain prior events, and arm before H2C; it cannot reject a stale RFK report.
struct RfkReportTracker {
    std::optional<RfkReport> last;
    bool observe(const std::uint8_t* bytes, std::size_t size) {
        const auto report = decode_rfk_report(bytes, size);
        if (!report) return false;
        if (complete() && (last->state != report->state || last->version != report->version ||
                           last->opaque_extension != report->opaque_extension))
            throw std::invalid_argument("Conflicting RFK terminal report");
        last = report;
        return true;
    }
    bool complete() const { return last && last->state != RfkReportState::start; }
    bool success() const { return complete() && last->state == RfkReportState::ok; }
};
} // namespace archer
