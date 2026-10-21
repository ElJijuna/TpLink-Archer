#pragma once
#include "archer/channel_init.hpp"
namespace archer {
// Pure ordered plan. Scheduler and PPDU operations are mandatory coordinator
// callbacks, not register guesses. All BB addresses here are physical.
enum class ChannelTransitionKind { pause_scheduler, ppdu_disable, ppdu_enable,
    masked_bb_write, delay_us, full_rf_write, resume_scheduler };
struct ChannelTransitionOp {
    ChannelTransitionKind kind;
    unsigned address=0, mask=0, value=0, path=0;
};
inline std::vector<ChannelTransitionOp> channel1_reset_enter(const Channel1Context& context) {
    validate_channel1_context(context);
    using K=ChannelTransitionKind;
    return {{K::pause_scheduler},{K::ppdu_disable},
        {K::masked_bb_write,0x22800,2,0},{K::masked_bb_write,0x22900,2,0},
        {K::masked_bb_write,0x2e610,8,8},{K::masked_bb_write,0x2e710,8,8},
        {K::masked_bb_write,0x210fc,0x01000000,0x01000000},
        {K::masked_bb_write,0x210fc,0x02000000,0x02000000},
        {K::delay_us,0,0,40},{K::masked_bb_write,0x20520,1,1},
        {K::masked_bb_write,0x20c3c,0x200,0x200},{K::delay_us,0,0,1},
        {K::masked_bb_write,0x20704,2,0}};
}
inline std::vector<ChannelTransitionOp> channel1_reset_exit(const Channel1Context& context,
                                                           bool resume_network_tx=false) {
    validate_channel1_context(context);
    using K=ChannelTransitionKind;
    std::vector<ChannelTransitionOp> result{{K::ppdu_enable},
        {K::masked_bb_write,0x210fc,0x01000000,0},
        {K::masked_bb_write,0x210fc,0x02000000,0},
        {K::masked_bb_write,0x22800,2,2},{K::masked_bb_write,0x22900,2,2},
        {K::masked_bb_write,0x2e610,8,0},{K::masked_bb_write,0x2e710,8,0},
        {K::masked_bb_write,0x20704,2,2},{K::masked_bb_write,0x20520,1,0},
        {K::masked_bb_write,0x20c3c,0x200,0}};
    // Diagnostics keep scheduler TX paused. Full source flow resumes saved TX.
    if(resume_network_tx) result.push_back({K::resume_scheduler});
    return result;
}
struct Channel1PostRfPlan {
    std::vector<ChannelTransitionOp> operations;
    std::array<unsigned,2> selected_table;
    // Host MCC update required before the matching PRE_NOTIFY. These are intent
    // values to commit only after successful post-RF programming.
    unsigned channel=1, band=0, bandwidth_mhz=20, rf18=1;
};
inline Channel1PostRfPlan channel1_post_rf_plan(const Channel1Context& context,
    const std::array<unsigned,2>& selected_table,
    const std::array<std::uint32_t,2>& full_rf01_snapshot) {
    validate_channel1_context(context);
    for(unsigned path=0;path<2;++path)
        if(selected_table[path]>2 || full_rf01_snapshot[path]>0xfffff)
            throw std::runtime_error("Invalid selected RFK MCC slot or RF01 snapshot");
    using K=ChannelTransitionKind;
    Channel1PostRfPlan result;result.selected_table=selected_table;
    for(unsigned path=0;path<2;++path){
        const auto table=selected_table[path], offset=path*0x100;
        result.operations.push_back({K::masked_bb_write,0x28104+offset,0x80000000,0x80000000});
        result.operations.push_back({K::masked_bb_write,0x28104+offset,3,table});
        result.operations.push_back({K::masked_bb_write,0x28104+offset,0x300,table<<8});
        result.operations.push_back({K::full_rf_write,1,0xfffff,
            merge_channel_rf_field(full_rf01_snapshot[path],0xe0000,4|table),path});
        // Linux reads selector bits after writing them. The coordinator must
        // verify each masked write before these source-equivalent LUT writes.
        result.operations.push_back({K::masked_bb_write,0x28154+offset,8,(table&1)<<3});
        result.operations.push_back({K::masked_bb_write,0x28154+offset,0x20,((table>>1)&1)<<5});
    }
    return result;
}
// Source lookup's result for a freshly zeroed RFK MCC state, with exactly one
// active entity (channel1), is slot0 for both paths. Reused state requires the
// actual source lookup, not this shortcut.
inline std::array<unsigned,2> channel1_fresh_mcc_slots(bool freshly_zeroed_mcc,
                                                     bool sole_active_entity) {
    if(!freshly_zeroed_mcc || !sole_active_entity)
        throw std::runtime_error("RFK MCC lookup requires actual channel history");
    return {0,0};
}
}
