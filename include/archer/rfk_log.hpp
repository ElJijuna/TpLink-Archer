#pragma once
#include "firmware_channel.hpp"
#include <array>
namespace archer {
struct RfkLogChunk {
    unsigned type;
    std::vector<std::uint8_t> content;
};
struct RfkLog {
    unsigned function;
    std::vector<RfkLogChunk> chunks;
};
inline std::optional<RfkLog> decode_rfk_log(const std::uint8_t* bytes,std::size_t size) {
    const auto header=decode_c2h(bytes,size);
    if(header.category!=2 || header.command_class!=8)return std::nullopt;
    RfkLog log{header.function,{}};
    std::size_t cursor=8;
    while(cursor<size) {
        if(size-cursor<3)throw std::invalid_argument("Truncated RFK log chunk header");
        const unsigned type=bytes[cursor];
        const unsigned length=bytes[cursor+1]|(unsigned(bytes[cursor+2])<<8);
        cursor+=3;
        if(type>1 || !length || length>size-cursor)
            throw std::invalid_argument("Unknown or truncated RFK log chunk");
        log.chunks.push_back({type,std::vector<std::uint8_t>(bytes+cursor,bytes+cursor+length)});
        cursor+=length;
    }
    if(log.chunks.empty())throw std::invalid_argument("Empty RFK log");
    return log;
}
struct DackReportV2 {
    unsigned calibration_version, info_version;
    // Source offsets106..112: ADDCK, CDACK, DADCK, MSBK, ADGAINK, WBADCDCK, DRCK.
    std::array<unsigned,7> timeout_flags;
    unsigned dack_fail;
    std::array<unsigned,2> wbdck;
    unsigned rck;
    std::optional<unsigned> adgaink_ex;
    std::vector<std::uint8_t> raw;
    bool has_failure_flags() const {
        return dack_fail || std::any_of(timeout_flags.begin(),timeout_flags.end(),
                                       [](unsigned flags){return flags!=0;});
    }
    // This describes report flags only, never transport completion or RF quality.
    bool flags_clear() const {return !has_failure_flags();}
};
struct RxdckReport {
    unsigned version;
    std::array<unsigned,2> band, bandwidth, channel, timeout;
    bool clean_initial_channel1() const {
        return band==std::array<unsigned,2>{0,0} && bandwidth==std::array<unsigned,2>{0,0} &&
               channel==std::array<unsigned,2>{1,1} && timeout==std::array<unsigned,2>{0,0};
    }
};
inline std::vector<RxdckReport> decode_rxdck_log_reports(const std::uint8_t* bytes,std::size_t size) {
    const auto log=decode_rfk_log(bytes,size);
    if(!log || log->function!=3)return {};
    std::vector<RxdckReport> reports;
    for(const auto& chunk:log->chunks)if(chunk.type==1) {
        const auto& b=chunk.content;
        if(b.size()!=9)throw std::invalid_argument("Unsupported RXDCK log extent");
        reports.push_back({b[0],{b[1],b[2]},{b[3],b[4]},{b[5],b[6]},{b[7],b[8]}});
    }
    return reports;
}
inline DackReportV2 decode_dack_report_v2(const RfkLogChunk& chunk) {
    const auto& b=chunk.content;
    // Older source117 bytes; pinned source118 appends only adgaink_ex_d.
    // Report version2 is independently emitted in the physical stage30 capture.
    if(chunk.type!=1 || (b.size()!=117 && b.size()!=118) || b[1]!=2)
        throw std::invalid_argument("Unsupported DACK report format; quality unknown");
    DackReportV2 report{b[0],b[1],{},b[113],{b[114],b[115]},b[116],std::nullopt,b};
    for(unsigned i=0;i<7;++i)report.timeout_flags[i]=b[106+i];
    if(b.size()==118)report.adgaink_ex=b[117];
    return report;
}
inline std::vector<DackReportV2> decode_dack_log_reports(const std::uint8_t* bytes,std::size_t size) {
    const auto log=decode_rfk_log(bytes,size);
    if(!log || log->function!=2) return {};
    std::vector<DackReportV2> reports;
    for(const auto& chunk:log->chunks)if(chunk.type==1)reports.push_back(decode_dack_report_v2(chunk));
    return reports;
}
} // namespace archer
