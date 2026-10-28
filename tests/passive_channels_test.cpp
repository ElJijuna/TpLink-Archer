#include "archer/passive_channels.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
archer::Channel1BbInputs board(){
    archer::Channel1BbInputs b;b.efuse_offset_valid=false;
    for(auto& g:b.gain){
        for(auto& v:g.lna_gain)v=0;for(auto& v:g.lna_op1db)v=0;for(auto& v:g.tia_gain)v=0;
        for(auto& v:g.tia_lna_op1db)v=0;for(auto& v:g.rpl160)v=0;for(auto& v:g.rpl80)v=0;
        for(auto& v:g.rpl40)v=0;for(auto& v:g.rpl20)v=0;
    }return b;
}
int main(){
    const archer::Channel1Inputs ready{true,true,true,true,true};const auto b=board();
    const auto baseline=archer::channel1_bb_plan({},ready,b);
    for(unsigned ch:{1u,6u,11u}){
        const auto p=archer::passive_channel_bb_plan(ch,ready,b);assert(p.size()==baseline.size());
        unsigned changes=0;
        for(unsigned i=0;i<p.size();++i){assert(p[i].address==baseline[i].address&&p[i].mask==baseline[i].mask);changes+=p[i].value!=baseline[i].value;}
        assert(changes==(ch==1?0u:5u));
        auto value=[&](unsigned a,unsigned mask){for(auto& op:p)if(op.address==a&&op.mask==mask)return op.value;throw std::runtime_error("missing field");};
        assert(value(0x26b4c,0x1fff)==(ch==1?2412u:ch==6?2437u:2462u));
        assert(value(0x26b50,0x7f)==(ch==1?109u:ch==6?108u:106u));
        assert(value(0x26758,0x7ffff)==(ch==1?0x1fe4fu:ch==6?0x20399u:0x208e3u));
        assert(value(0x2675c,0x7ffff)==(ch==1?0x2bdacu:ch==6?0x2c4f2u:0x2cc38u));
        assert(value(0x20734,0xff0000)==ch<<16);
        const std::array<std::uint32_t,4> snapshot{0xfffff,0xabcde,0x53421,0x12345};
        const auto rf=archer::passive_channel_rf_plan(ch,ready,snapshot,1);
        for(unsigned i=0;i<4;++i){assert((rf[i].full_value&0x31fff)==ch);assert((rf[i].full_value&~0x31fff)==(snapshot[i]&~0x31fff));assert(rf[i].delay_after_us==100);}
        const auto post=archer::passive_channel_post_rf_plan(ch,{1,2},{0x12345,0xabcde});
        assert(post.channel==ch&&post.rf18==ch&&post.selected_table[1]==2);
    }
    for(unsigned bad:{0u,2u,13u,14u,36u})rejects([&]{archer::passive_channel_bb_plan(bad,ready,b);});
    rejects([&]{archer::passive_channel_rf_plan(6,ready,{0,0,0,0x100000},1);});
    rejects([&]{archer::passive_channel_bb_plan(6,{},b);});
    rejects([&]{archer::passive_channel_post_rf_plan(6,{3,0},{0,0});});
    // Existing history must be searched; target6 is slot1 even though slot0 is idle.
    assert(archer::passive_mcc_lookup({1,6,11},6,{6})==1);
    assert(archer::passive_mcc_lookup({1,0,0},6,{1,6})==1);
    assert(archer::passive_mcc_lookup({1,0,0},6,{6})==0);
    assert(archer::passive_mcc_lookup({2,3,4},6,{2,3,4})==0);
    rejects([&]{archer::passive_mcc_lookup({1,0,0},6,{});});
    std::cout<<"Passive 1/6/11 source deltas, RF preservation and MCC history lookup: OK\n";
}
