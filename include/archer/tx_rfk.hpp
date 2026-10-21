#pragma once
#include "rfk_log.hpp"
#include "tssi_protocol.hpp"
namespace archer {
// These predicates assess documented firmware flags, not measured EVM or power.
struct IqkReport {
    std::array<unsigned,2> tx_fail,rx_fail,band,channel,bandwidth;
    unsigned version,phy,status;
    std::vector<std::uint8_t> raw;
    bool flags_clear() const {return tx_fail==std::array<unsigned,2>{0,0} && rx_fail==std::array<unsigned,2>{0,0};}
    bool matches(const RfkChannel& c,unsigned p) const {
        return phy==p && band==std::array<unsigned,2>{c.band,c.band} &&
            channel==std::array<unsigned,2>{c.channel,c.channel} && bandwidth==std::array<unsigned,2>{c.bandwidth,c.bandwidth};
    }
};
inline IqkReport decode_iqk_report(const RfkLogChunk& chunk) {
    const auto& b=chunk.content;
    // Both exact historical184-byte and current516-byte layouts share this32-byte prefix.
    if(chunk.type!=1 || (b.size()!=184 && b.size()!=516))
        throw std::invalid_argument("Unsupported IQK log extent; quality unknown");
    for(unsigned i=0;i<21;++i)if(b[i]>1)throw std::invalid_argument("Non-boolean IQK report flag");
    return {{b[0],b[1]},{b[2],b[3]},{b[21],b[22]},{b[23],b[24]},
            {b[25],b[26]},b[28],b[29],b[30],b};
}
struct DpkReport {
    unsigned version;
    std::array<unsigned,2> band,bandwidth,channel,path_ok,timeout,rxbb_overflow;
    std::vector<std::uint8_t> raw;
    bool error_flags_clear() const {return timeout==std::array<unsigned,2>{0,0} &&
        rxbb_overflow==std::array<unsigned,2>{0,0};}
    // RTL8922A v13 path_ok producer semantics are unavailable. Other chips use
    // either a boolean or calibrated-set bitmap; never reinterpret4 as true.
    std::optional<bool> path_success() const {
        if(path_ok[0]==0 || path_ok[1]==0)return false;
        return std::nullopt;
    }
    bool flags_clear() const {return error_flags_clear() && path_success().value_or(false);}
    bool matches(const RfkChannel& c) const {return band==std::array<unsigned,2>{c.band,c.band} &&
        channel==std::array<unsigned,2>{c.channel,c.channel} && bandwidth==std::array<unsigned,2>{c.bandwidth,c.bandwidth};}
};
inline DpkReport decode_dpk_report(const RfkLogChunk& chunk) {
    const auto& b=chunk.content;
    if(chunk.type!=1 || b.size()!=34)throw std::invalid_argument("Unsupported DPK log extent; quality unknown");
    return {b[0],{b[3],b[4]},{b[5],b[6]},{b[7],b[8]},{b[9],b[10]},
            {b[29],b[30]},{b[31],b[32]},b};
}
struct TxgapkReport {
    unsigned version,ok,check_id,check_count;
    std::optional<unsigned> dual_band_ok;
    std::optional<std::array<std::uint32_t,2>> stage;
    std::optional<std::array<unsigned,2>> failcode;
    std::vector<std::uint8_t> raw;
    // Historical84-byte source exposes only the explicit overall success flag.
    // Absent failcodes remain unknown, not synthesized zero.
    bool flags_clear() const {return ok==1 && (!failcode || *failcode==std::array<unsigned,2>{0,0});}
};
inline TxgapkReport decode_txgapk_report(const RfkLogChunk& chunk) {
    const auto& b=chunk.content;
    if(chunk.type!=1 || (b.size()!=84 && b.size()!=100))
        throw std::invalid_argument("Unsupported TXGAPK log extent; quality unknown");
    TxgapkReport r{b[82],b[80],b[81],channel_u32(b.data()+8),
                  std::nullopt,std::nullopt,std::nullopt,b};
    if(b.size()==100) {
        r.dual_band_ok=b[83];
        r.stage=std::array<std::uint32_t,2>{channel_u32(b.data()+84),channel_u32(b.data()+88)};
        r.failcode=std::array<unsigned,2>{unsigned(b[92])|(unsigned(b[93])<<8),
                                        unsigned(b[94])|(unsigned(b[95])<<8)};
    }
    return r;
}
struct TssiReport {
    std::array<int,16> alignment_power;
    std::array<unsigned,16> alignment_code;
    std::array<unsigned,4> alignment_state,default_txagc_offset;
    // No success interpretation: pinned Linux only prints these state bytes.
};
inline TssiReport decode_tssi_report(const RfkLogChunk& chunk) {
    const auto& b=chunk.content;
    if(chunk.type!=1 || b.size()!=56)throw std::invalid_argument("Unsupported TSSI log extent; quality unknown");
    TssiReport r{};
    for(unsigned i=0;i<16;++i){r.alignment_power[i]=tssi_signed(b[i]);r.alignment_code[i]=(unsigned(b[16+i])<<8)|b[32+i];}
    for(unsigned i=0;i<4;++i){r.alignment_state[i]=b[48+i];r.default_txagc_offset[i]=b[52+i];}
    return r;
}
template<class Report,class Decoder> inline std::vector<Report> decode_tx_rfk_reports(
        const std::uint8_t* bytes,std::size_t size,unsigned function,Decoder decode) {
    const auto log=decode_rfk_log(bytes,size);if(!log || log->function!=function)return {};
    std::vector<Report> out;
    for(const auto& chunk:log->chunks)if(chunk.type==1)out.push_back(decode(chunk));
    return out;
}
inline std::vector<IqkReport> decode_iqk_log_reports(const std::uint8_t* b,std::size_t n){return decode_tx_rfk_reports<IqkReport>(b,n,0,decode_iqk_report);}
inline std::vector<DpkReport> decode_dpk_log_reports(const std::uint8_t* b,std::size_t n){return decode_tx_rfk_reports<DpkReport>(b,n,1,decode_dpk_report);}
inline std::vector<TssiReport> decode_tssi_log_reports(const std::uint8_t* b,std::size_t n){return decode_tx_rfk_reports<TssiReport>(b,n,4,decode_tssi_report);}
inline std::vector<TxgapkReport> decode_txgapk_log_reports(const std::uint8_t* b,std::size_t n){return decode_tx_rfk_reports<TxgapkReport>(b,n,5,decode_txgapk_report);}
// Reproduce pinned Linux freshly allocated RTL8922A host state. This is
// NOT a claim about factory trim. Its efuse parser never assigns these arrays.
inline TssiTrimData rtl8922a_linux_host_default_tssi_trim() {
    return TssiTrimData{std::array<std::array<int,8>,2>{},std::array<std::array<int,16>,2>{}};
}
struct TxRfkPrerequisites {
    bool cmac_initialized,channel_applied,scheduler_stopped,rf_not_tx_both_paths,coexistence_prepared;
};
inline void require_tx_rfk_prerequisites(const TxRfkPrerequisites& p) {
    if(!p.cmac_initialized || !p.channel_applied || !p.scheduler_stopped ||
       !p.rf_not_tx_both_paths || !p.coexistence_prepared)
        throw std::invalid_argument("TX RFK requires CMAC/channel, stopped scheduler, both RF paths out of TX and coexistence context");
}
inline std::vector<std::uint8_t> encode_prepared_tssi_normal_h2c(unsigned sequence,
        RfkFirmwareVersion version,unsigned phy,const RfkChannel& channel,unsigned cv,unsigned thermal_group,
        const TssiBoardData& board,const TssiTrimData& trim,const TssiTracking& tracking,
        const TxRfkPrerequisites& prerequisites) {
    require_tx_rfk_prerequisites(prerequisites);
    return encode_h2c(2,0xb,0,sequence,prepare_tssi_normal_payload(version,phy,channel,cv,thermal_group,board,trim,tracking));
}
inline std::vector<std::uint8_t> encode_prepared_tssi_scan_h2c(unsigned sequence,
        RfkFirmwareVersion version,unsigned phy,const RfkChannel& channel,unsigned cv,unsigned thermal_group,
        const TssiBoardData& board,const TssiTrimData& trim,const TssiTracking& tracking,
        const TxRfkPrerequisites& prerequisites) {
    require_tx_rfk_prerequisites(prerequisites);
    auto payload=prepare_tssi_normal_payload(version,phy,channel,cv,thermal_group,board,trim,tracking);
    payload[298]=1; // RTW89_TSSI_SCAN: first entity/band-change setup.
    return encode_h2c(2,0xb,0,sequence,payload);
}
} // namespace archer
