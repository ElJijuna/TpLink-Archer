#pragma once
#include "archer/register_mailbox.hpp"
#include <vector>
#include <array>
#include <optional>
#include <algorithm>
namespace archer {
// Intended context only. Construction never establishes applied RF state.
struct Channel1Context {
    unsigned primary_channel = 1;
    unsigned center_channel = 1;
    unsigned center_frequency_mhz = 2412;
    unsigned bandwidth_mhz = 20;
    unsigned primary_subband_index = 0;
    unsigned mac_index = 0;
    unsigned phy_index = 0;
    bool band_2ghz = true;
    bool dbcc = false;
};
struct Channel1Inputs {
    bool mac_bb_rf_initialized = false;
    bool scheduler_paused_and_reset_owned = false;
    bool board_gain_tables_validated = false;
    bool efuse_gain_and_trim_decisions_validated = false;
    bool tx_power_context_validated = false;
};
inline void validate_channel1_context(const Channel1Context& c) {
    if(c.primary_channel != 1 || c.center_channel != 1 || c.center_frequency_mhz != 2412 ||
       c.bandwidth_mhz != 20 || c.primary_subband_index != 0 || c.mac_index || c.phy_index ||
       !c.band_2ghz || c.dbcc)
        throw std::runtime_error("Only intended PHY0/MAC0 SCC 2.4GHz channel1 20MHz context is supported");
}
struct Channel1Assessment {
    std::vector<std::string> blockers;
    bool executable() const { return blockers.empty(); }
};
inline Channel1Assessment assess_channel1_setup(const Channel1Context& context,
                                                const Channel1Inputs& inputs) {
    validate_channel1_context(context);
    Channel1Assessment result;
    if(!inputs.mac_bb_rf_initialized) result.blockers.emplace_back("Full MAC/BB/RF initialization unverified");
    if(!inputs.scheduler_paused_and_reset_owned) result.blockers.emplace_back("Scheduler/reset channel transition ownership unverified");
    if(!inputs.board_gain_tables_validated) result.blockers.emplace_back("RFE1 BB gain table inputs unverified");
    if(!inputs.efuse_gain_and_trim_decisions_validated) result.blockers.emplace_back("Efuse gain and RF trim decisions unverified");
    if(!inputs.tx_power_context_validated) result.blockers.emplace_back("Channel TX power context unverified");
    // Evidence flags cannot bypass missing complete transaction coordination.
    result.blockers.emplace_back("Complete reset/MAC/post-RF transaction coordinator is not implemented");
    return result;
}
inline void initialize_channel1(RegisterBus&, const Channel1Context& context,
                                const Channel1Inputs& inputs) {
    const auto assessment = assess_channel1_setup(context, inputs);
    std::string message = "Channel1 setup refused before I/O: ";
    for(const auto& blocker : assessment.blockers) message += blocker + "; ";
    throw std::runtime_error(message);
}

struct Channel1PathGain {
    std::array<std::optional<int>,7> lna_gain, lna_op1db;
    std::array<std::optional<int>,2> tia_gain;
    std::array<std::optional<int>,8> tia_lna_op1db, rpl160;
    std::array<std::optional<int>,4> rpl80;
    std::array<std::optional<int>,2> rpl40;
    std::array<std::optional<int>,1> rpl20;
};
struct Channel1MacWrite {
    std::uint32_t address, mask, value;
    unsigned width;
};
inline std::array<Channel1MacWrite,6> channel1_mac_plan(const Channel1Context& context) {
    validate_channel1_context(context);
    return {{{0x10010,7,0,1},{0x10088,0xffffffff,0x10000,4},
        {0x10828,0x13,0,1},{0x10828,0x10,0x10,1},
        {0x1033c,0x7f,0x3f,4},{0x10370,0x7f0000,0x3e0000,4}}};
}
struct Channel1BbInputs {
    std::array<Channel1PathGain,2> gain;
    // Explicit false means source's offset_valid=false skip decision was proven.
    std::optional<bool> efuse_offset_valid;
    std::array<std::optional<int>,2> efuse_cck, efuse_ofdm;
};
struct Channel1BbWrite {
    std::uint32_t address, mask, value; // physical BB address; value already shifted/masked
};
inline std::vector<Channel1BbWrite> channel1_bb_plan(const Channel1Context& context,
    const Channel1Inputs& prerequisites, const Channel1BbInputs& inputs) {
    validate_channel1_context(context);
    if(!prerequisites.mac_bb_rf_initialized || !prerequisites.scheduler_paused_and_reset_owned ||
       !prerequisites.board_gain_tables_validated || !prerequisites.efuse_gain_and_trim_decisions_validated ||
       !prerequisites.tx_power_context_validated || !inputs.efuse_offset_valid.has_value())
        throw std::runtime_error("Channel BB plan requires complete lifecycle/board/efuse prerequisites");
    std::vector<Channel1BbWrite> plan;
    auto cell=[](const std::optional<int>& value) {
        if(!value || *value < -128 || *value > 127)
            throw std::runtime_error("Missing or invalid signed8 channel gain cell");
        return *value;
    };
    auto add=[&](unsigned address,unsigned mask,int value) {
        unsigned shift=0;while(!(mask&(1u<<shift)))++shift;
        plan.push_back({0x20000+address,mask,(static_cast<unsigned>(value)<<shift)&mask});
    };
    add(0x6758,0x7ffff,0x1fe4f);add(0x675c,0x7ffff,0x2bdac);
    constexpr unsigned bb_gain_lna[][2] = {{0x409c,0xFF00},{0x409c,0xFF000000},{0x40a0,0xFF00},{0x40a0,0xFF000000},{0x40a4,0xFF00},{0x40a4,0xFF000000},{0x40a8,0xFF00}};
    constexpr unsigned bb_gain_tia[][2] = {{0x4054,0x7FC0000},{0x4058,0x1FF}};
    constexpr unsigned bb_op1db_lna[][2] = {{0x40ac,0xFF00},{0x40ac,0xFF0000},{0x40ac,0xFF000000},{0x40b0,0xFF},{0x40b0,0xFF00},{0x40b0,0xFF0000},{0x40b0,0xFF000000}};
    constexpr unsigned bb_op1db_tia_lna[][2] = {{0x40b4,0xFF0000},{0x40b4,0xFF000000},{0x40b8,0xFF},{0x40b8,0xFF00},{0x40b8,0xFF0000},{0x40b8,0xFF000000},{0x40bc,0xFF},{0x40bc,0xFF00}};
    constexpr unsigned rpl[][2]={{0x41e8,0xff00},{0x41e8,0xff0000},{0x41e8,0xff000000},
        {0x41ec,0xff},{0x41ec,0xff00},{0x41ec,0xff0000},{0x41ec,0xff000000},{0x41f0,0xff},
        {0x41f4,0xff},{0x41f4,0xff00},{0x41f4,0xff0000},{0x41f4,0xff000000},
        {0x41f0,0xff0000},{0x41f0,0xff000000},{0x41f0,0xff00}};
    for(unsigned path=0;path<2;++path){
        const auto& g=inputs.gain[path];const unsigned offset=path*0x400;
        for(unsigned i=0;i<7;++i)add(bb_gain_lna[i][0]+offset,bb_gain_lna[i][1],cell(g.lna_gain[i]));
        for(unsigned i=0;i<2;++i)add(bb_gain_tia[i][0]+offset,bb_gain_tia[i][1],cell(g.tia_gain[i]));
        for(unsigned i=0;i<7;++i)add(bb_op1db_lna[i][0]+offset,bb_op1db_lna[i][1],cell(g.lna_op1db[i]));
        for(unsigned i=0;i<8;++i)add(bb_op1db_tia_lna[i][0]+offset,bb_op1db_tia_lna[i][1],cell(g.tia_lna_op1db[i]));
        for(unsigned i=0;i<8;++i)add(rpl[i][0]+offset,rpl[i][1],cell(g.rpl160[i]));
        for(unsigned i=0;i<4;++i)add(rpl[i+8][0]+offset,rpl[i+8][1],cell(g.rpl80[i]));
        for(unsigned i=0;i<2;++i)add(rpl[i+12][0]+offset,rpl[i+12][1],cell(g.rpl40[i]));
        add(rpl[14][0]+offset,rpl[14][1],cell(g.rpl20[0]));
    }
    add(0x4160,1u<<26,1);add(0x4560,1u<<26,1);
    if(*inputs.efuse_offset_valid){
        for(unsigned path=0;path<2;++path){
            int value=-cell(inputs.efuse_cck[path]);
            if(value==128)value=-128; // source stores negation back into s8
            const unsigned fraction=static_cast<unsigned>(value)&3;
            add(0x672c,0xf,fraction?(4-fraction)<<1:0);
            add(0x672c,0xf0,fraction?(4-fraction)<<1:0);
            // Arithmetic signed shift in Linux; floor division avoids host dependence.
            value=value>=0?value/4:-((-value+3)/4);
            add(0x6750,0xff,value+(fraction?1:0)+0xdc);
            value=cell(inputs.efuse_ofdm[path]);
            const unsigned offset=path*0x400;
            add(0x40c8+offset,0xff000000,value+0xf8);
            value*=-4;
            const int v1=std::clamp(value,-128,127);value-=v1;
            const int v2=std::clamp(value,-128,127);value-=v2;
            // Linux declares v3 s8 after clamp; preserve that exact saturation.
            const int v3=std::clamp(value,-128,127);
            add(0x41e8+offset,0xff,v1);add(0x41f8+offset,0xff,v2);add(0x41f8+offset,0xff00,v3);
            add(0x41f8+offset,0xff0000,v1);add(0x4208+offset,0xff0000,v2);add(0x4208+offset,0xff000000,v3);
        }
    }
    add(0x6b4c,0x1fff,2412);add(0x6b50,0x7f,109);
    constexpr unsigned coeff[]={0x3a63ca,0x2a833f,0x1491f8,0x03c0b0,0xfccff1,0xfccfc3,0xfebfdc,0xffdff7};
    for(unsigned i=0;i<8;++i)add(0x6684+4*i,0xffffff,coeff[i]);
    add(0x0734,0xff0000,1);
    add(0x6b54,0x7000,0);add(0x6b50,0xc0000000,0);add(0x6b54,0xf00,0);
    add(0x625c,0xc0000000,1);add(0xe44c,1,0);add(0xe54c,1,0);add(0x6b4c,0x80000000,0);
    add(0x0520,1,0);add(0x0700,1u<<5,1);add(0x0c80,1u<<31,0);
    add(0x4d34,1,0);add(0x41a0,0x1000,0);add(0x41ac,0x1000,0);
    add(0x45a0,0x1000,0);add(0x45ac,0x1000,0);add(0x0704,2,1);
    add(0xe60c,0x10000,0);add(0xe60c,0x10000,1);add(0xe70c,0x10000,0);add(0xe70c,0x10000,1);
    return plan;
}

// Offline subplan only: not a complete channel transaction or an applied token.
// Snapshot order is path A 0x18, A 0x10018, B 0x18, B 0x10018.
struct Channel1RfWrite {
    unsigned path;
    std::uint32_t address, full_value;
    unsigned delay_after_us;
};
inline std::array<Channel1RfWrite, 4> channel1_rf_tuning_plan(
    const Channel1Context& context, const Channel1Inputs& inputs,
    const std::array<std::uint32_t, 4>& full_rf_snapshot, unsigned chip_cut) {
    validate_channel1_context(context);
    if(chip_cut != 1 || !inputs.mac_bb_rf_initialized ||
       !inputs.scheduler_paused_and_reset_owned || !inputs.board_gain_tables_validated ||
       !inputs.efuse_gain_and_trim_decisions_validated || !inputs.tx_power_context_validated)
        throw std::runtime_error("RF tuning subplan requires CV1 and validated complete channel prerequisites");
    // Validate every read before returning any write, unlike a streaming partial
    // update. No defaults are substituted for failed RF reads.
    for(const auto value : full_rf_snapshot)
        if(value > 0xfffff) throw std::runtime_error("Invalid full RF channel snapshot");
    std::array<Channel1RfWrite, 4> writes{};
    for(unsigned i=0; i<writes.size(); ++i)
        writes[i] = {i/2, i%2 ? 0x10018u : 0x18u,
                     (full_rf_snapshot[i] & ~0x31fffu) | 1u, 100};
    return writes;
}
// Correct V2 RF mask operation for a future validated channel executor. Values
// are unshifted field values; read sentinel handling belongs at the RF boundary.
inline std::uint32_t merge_channel_rf_field(std::uint32_t original, std::uint32_t mask,
                                           std::uint32_t field) {
    if(original > 0xfffff || !mask || mask > 0xfffff)
        throw std::invalid_argument("Invalid 20-bit RF source or mask");
    unsigned shift = 0;
    while(!(mask & (1u << shift))) ++shift;
    const auto bits = mask >> shift;
    if((bits & (bits + 1)) || field > bits)
        throw std::invalid_argument("RF field requires a contiguous mask and representable value");
    return (original & ~mask) | ((field << shift) & mask);
}
}
