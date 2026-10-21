#pragma once
#include "archer/channel_init.hpp"
#include <cstdlib>
namespace archer {
struct Channel1BoardTrim {
    bool pa_and_pad_programmed=false, thermal_programmed=false;
    std::array<unsigned,2> pa_raw{}, pad_raw{};
    std::array<int,2> thermal_trim{};
};
struct BoardTrimRfWrite { unsigned path,address,mask,value; };
inline std::vector<BoardTrimRfWrite> channel1_board_trim_plan(const Channel1Context& context,
    const Channel1BoardTrim& trim, unsigned aid, unsigned rfe, unsigned cv) {
    validate_channel1_context(context);
    if(aid!=0x6933 || rfe!=1 || cv!=1)throw std::invalid_argument("Unbound board trim identity");
    for(unsigned i=0;i<2;++i)
        if(trim.pa_raw[i]>255 || trim.pad_raw[i]>255 || trim.thermal_trim[i]<-63 || trim.thermal_trim[i]>63)
            throw std::invalid_argument("Invalid PHY trim value");
    std::vector<BoardTrimRfWrite> result;
    if(!trim.pa_and_pad_programmed)return result;
    // Source applies both paths' PA first, then both paths' PAD. Thermal trim
    // affects temperature compensation elsewhere; power_trim performs no thermal RF write.
    for(unsigned i=0;i<2;++i){result.push_back({i,0x60,0xf,trim.pa_raw[i]&15});
        result.push_back({i,0x60,0xf00,trim.pa_raw[i]>>4});}
    for(unsigned i=0;i<2;++i){result.push_back({i,0x60,0xf0,trim.pad_raw[i]&15});
        result.push_back({i,0x60,0xf000,trim.pad_raw[i]>>4});}
    return result;
}
// rtw8922a_chip_ops.init_txpwr_unit=NULL. No substitute initialization writes.
inline std::vector<Channel1BbWrite> channel1_txpwr_unit_plan(const Channel1Context& c){
    validate_channel1_context(c);return {};
}
struct Channel1PowerContext {
    // Result of source antenna-gain/regulatory/firmware-feature decision, in S(8,3).
    std::optional<int> antenna_offset_bb;
    bool byrate_applied=false, offset_applied=false, shape_applied=false,
         regulatory_limit_applied=false, ru_limit_applied=false, sar_applied=false;
    bool complete()const{return antenna_offset_bb.has_value() && byrate_applied && offset_applied &&
        shape_applied && regulatory_limit_applied && ru_limit_applied && sar_applied;}
};
// Exact PHY0 CV1 source subset set_txpwr_diff + set_txpwr_ref. Does not install
// regulatory limits or authorize scheduler TX; caller must provide those separately.
inline std::vector<Channel1BbWrite> channel1_txpwr_reference_plan(const Channel1Context& c,
    const Channel1PowerContext& inputs){
    validate_channel1_context(c);
    if(!inputs.antenna_offset_bb || *inputs.antenna_offset_bb < -127 || *inputs.antenna_offset_bb >127)
        throw std::invalid_argument("Actual antenna gain power offset required");
    const int offset=*inputs.antenna_offset_bb, mag=std::abs(offset);
    std::vector<Channel1BbWrite> result;
    for(unsigned path=0;path<2;++path){
        const bool decrement=path==0?offset<=0:offset>0;
        const int ref=decrement?-(mag/2):0, k=decrement?18+mag:18;
        result.push_back({0x2e628+path*0x100,0x1ff,static_cast<unsigned>(ref)&0x1ff});
        result.push_back({0x2e628+path*0x100,0x3fe00,(static_cast<unsigned>(ref)<<9)&0x3fe00});
        result.push_back({0x2e6a0+path*0x100,0x3ff00000,static_cast<unsigned>(k)<<20});
    }
    // MAC register addresses are physical; common write schema holds either space.
    result.push_back({0x11a20,0x3fe,0});result.push_back({0x11a20,0x7fc00,0});
    return result;
}
struct WorldDiagnosticPowerProfile {
    unsigned regd=0, sar_source=0, antenna_regd_enabled=0;
    bool scheduler_tx_paused=false, fresh_unconfigured_host_state=false;
};
inline void validate_world_diagnostic_power(const Channel1Context& c,
    const WorldDiagnosticPowerProfile& p) {
    validate_channel1_context(c);
    if(p.regd || p.sar_source || p.antenna_regd_enabled || !p.scheduler_tx_paused ||
       !p.fresh_unconfigured_host_state)
        throw std::invalid_argument("World diagnostic power requires fresh unconfigured host state and paused TX");
}

}
