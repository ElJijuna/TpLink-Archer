#pragma once
#include "archer/channel_transition.hpp"
namespace archer {
// Pure planning only: these plans do not prove calibration or authorize TX.
// Narrow scan candidates: PHY0/MAC0 SCC, 2.4GHz, 20MHz, channels 1/6/11.
inline unsigned passive_channel_frequency(unsigned channel){
    if(channel!=1&&channel!=6&&channel!=11)throw std::invalid_argument("Passive channel plan supports only 1/6/11");
    return 2407+5*channel;
}
inline std::vector<Channel1BbWrite> passive_channel_bb_plan(unsigned channel,
    const Channel1Inputs& prerequisites,const Channel1BbInputs& inputs){
    const auto frequency=passive_channel_frequency(channel);
    // Same gain band, offset band, width, CCK non-14 coefficients and spur
    // disable branch as channel1. Preserve the validated board/efuse input checks.
    auto plan=channel1_bb_plan({},prerequisites,inputs);
    const unsigned barker=channel==1?0x1fe4f:channel==6?0x20399:0x208e3;
    const unsigned cck=channel==1?0x2bdac:channel==6?0x2c4f2:0x2cc38;
    unsigned patched=0;
    for(auto& op:plan){
        if(op.address==0x26758&&op.mask==0x7ffff){op.value=barker;++patched;}
        if(op.address==0x2675c&&op.mask==0x7ffff){op.value=cck;++patched;}
        if(op.address==0x26b4c&&op.mask==0x1fff){op.value=frequency;++patched;}
        if(op.address==0x26b50&&op.mask==0x7f){op.value=(262144+frequency/2)/frequency;++patched;}
        if(op.address==0x20734&&op.mask==0xff0000){op.value=channel<<16;++patched;}
    }
    if(patched!=5)throw std::runtime_error("Channel baseline layout changed; re-audit source delta");
    return plan;
}
inline std::array<Channel1RfWrite,4> passive_channel_rf_plan(unsigned channel,
    const Channel1Inputs& prerequisites,const std::array<std::uint32_t,4>& snapshot,unsigned cut){
    (void)passive_channel_frequency(channel);
    auto plan=channel1_rf_tuning_plan({},prerequisites,snapshot,cut);
    for(auto& op:plan)op.full_value=(op.full_value&~0x31fffu)|channel;
    return plan;
}
// Source rtw89_rfk_chan_lookup: exact match wins even if an earlier slot is
// idle; otherwise first slot not used by any active entity, or fallback slot0.
// Previous slot channels must come from committed host history, not register
// guesses. Zero means an uninitialized source-zeroed descriptor.
inline unsigned passive_mcc_lookup(const std::array<unsigned,3>& previous,
    unsigned target,const std::vector<unsigned>& active_entities){
    auto valid=[](unsigned ch){return ch>=1&&ch<=13;};
    if(!valid(target)||active_entities.empty())throw std::invalid_argument("Actual MCC target/entity history required");
    for(auto ch:previous)if(ch&&!valid(ch))throw std::invalid_argument("Invalid previous MCC channel");
    for(auto ch:active_entities)if(!valid(ch))throw std::invalid_argument("Invalid active entity channel");
    std::optional<unsigned> idle;
    for(unsigned i=0;i<previous.size();++i){
        if(previous[i]&&previous[i]==target)return i;
        if(!idle&&(!previous[i]||std::find(active_entities.begin(),active_entities.end(),previous[i])==active_entities.end()))idle=i;
    }
    return idle.value_or(0);
}
inline Channel1PostRfPlan passive_channel_post_rf_plan(unsigned channel,
    const std::array<unsigned,2>& slots,const std::array<std::uint32_t,2>& rf01){
    (void)passive_channel_frequency(channel);
    auto plan=channel1_post_rf_plan({},slots,rf01);
    plan.channel=channel;plan.rf18=channel;return plan;
}
}
