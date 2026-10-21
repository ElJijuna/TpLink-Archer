#include "archer/channel_init.hpp"
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    unsigned calls=0;
    unsigned read(unsigned,unsigned)override{++calls;return 0;}
    void write(unsigned,unsigned,unsigned)override{++calls;}
    void pause()override{++calls;}
};
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
int main(){
    archer::Channel1Context context;
    assert(context.center_frequency_mhz==2412 && context.bandwidth_mhz==20);
    auto missing=archer::assess_channel1_setup(context,{});
    assert(!missing.executable() && missing.blockers.size()==6);
    archer::Channel1Inputs claimed{true,true,true,true,true};
    auto supplied=archer::assess_channel1_setup(context,claimed);
    assert(!supplied.executable() && supplied.blockers.size()==1);
    Bus bus;
    rejects([&]{archer::initialize_channel1(bus,context,{});});
    rejects([&]{archer::initialize_channel1(bus,context,claimed);});
    assert(bus.calls==0);
    for(unsigned field=0;field<9;++field){
        auto invalid=context;
        switch(field){
        case 0:invalid.primary_channel=2;break;
        case 1:invalid.center_channel=2;break;
        case 2:invalid.center_frequency_mhz=2417;break;
        case 3:invalid.bandwidth_mhz=40;break;
        case 4:invalid.primary_subband_index=1;break;
        case 5:invalid.mac_index=1;break;
        case 6:invalid.phy_index=1;break;
        case 7:invalid.band_2ghz=false;break;
        case 8:invalid.dbcc=true;break;
        }
        rejects([&]{archer::initialize_channel1(bus,invalid,claimed);});assert(bus.calls==0);
    }
    const auto mac=archer::channel1_mac_plan(context);
    assert(mac.size()==6 && mac[0].address==0x10010 && mac[0].width==1);
    assert(mac[1].value==0x10000 && mac[5].value==0x3e0000);
    archer::Channel1BbInputs bb;
    rejects([&]{archer::channel1_bb_plan(context,claimed,bb);});
    for(auto& g:bb.gain){
        for(auto& v:g.lna_gain)v=0;for(auto& v:g.lna_op1db)v=0;
        for(auto& v:g.tia_gain)v=0;for(auto& v:g.tia_lna_op1db)v=0;
        for(auto& v:g.rpl160)v=0;for(auto& v:g.rpl80)v=0;
        for(auto& v:g.rpl40)v=0;for(auto& v:g.rpl20)v=0;
    }
    bb.efuse_offset_valid=false;
    auto bbplan=archer::channel1_bb_plan(context,claimed,bb);
    assert(bbplan.size()==113 && bbplan[0].address==0x26758 && bbplan[0].value==0x1fe4f);
    assert(bbplan[2].address==0x2409c && bbplan[41].address==0x2449c);
    bb.gain[1].rpl160[7].reset();
    rejects([&]{archer::channel1_bb_plan(context,claimed,bb);});
    bb.gain[1].rpl160[7]=0;bb.efuse_offset_valid=true;
    rejects([&]{archer::channel1_bb_plan(context,claimed,bb);});
    bb.efuse_cck={-1,1};bb.efuse_ofdm={-128,127};
    bbplan=archer::channel1_bb_plan(context,claimed,bb);assert(bbplan.size()==133);
    for(const auto& op:bbplan)assert((op.value&~op.mask)==0 && op.address>=0x20000);
    const std::array<std::uint32_t,4> snapshot{{0xfffff,0xabcde,0x54321,0}};
    const auto rf=archer::channel1_rf_tuning_plan(context,claimed,snapshot,1);
    for(unsigned i=0;i<rf.size();++i){
        assert(rf[i].path==i/2 && rf[i].address==(i%2?0x10018u:0x18u));
        assert((rf[i].full_value&0x31fff)==1 && rf[i].delay_after_us==100);
        assert((rf[i].full_value&~0x31fffu)==(snapshot[i]&~0x31fffu));
    }
    for(unsigned i=0;i<snapshot.size();++i){
        auto bad=snapshot;bad[i]=0xffffffff;
        rejects([&]{archer::channel1_rf_tuning_plan(context,claimed,bad,1);});
    }
    rejects([&]{archer::channel1_rf_tuning_plan(context,{},snapshot,1);});
    rejects([&]{archer::channel1_rf_tuning_plan(context,claimed,snapshot,0);});
    assert(archer::merge_channel_rf_field(0xabcde,0x3f0,0x12)==((0xabcde&~0x3f0u)|0x120));
    assert(archer::merge_channel_rf_field(0,0xfffff,0xabcde)==0xabcde);
    rejects([]{archer::merge_channel_rf_field(0,0,1);});
    rejects([]{archer::merge_channel_rf_field(0,0x5,1);});
    rejects([]{archer::merge_channel_rf_field(0,0x30,4);});
    rejects([]{archer::merge_channel_rf_field(0x100000,1,0);});
    std::cout<<"Channel1 intent, missing prerequisites, no-I/O refusal and V2 RF mask semantics: OK\n";
}
