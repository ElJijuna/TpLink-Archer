#include "archer/channel_transition.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
int main(){
    using K=archer::ChannelTransitionKind;
    archer::Channel1Context context;
    auto enter=archer::channel1_reset_enter(context);
    assert(enter.size()==13 && enter[0].kind==K::pause_scheduler && enter[1].kind==K::ppdu_disable);
    assert(enter[6].address==0x210fc && enter[6].mask==0x01000000 && enter[6].value==0x01000000);
    assert(enter[8].kind==K::delay_us && enter[8].value==40);
    assert(enter[11].kind==K::delay_us && enter[11].value==1);
    assert(enter.back().address==0x20704 && enter.back().value==0);
    auto exit=archer::channel1_reset_exit(context);
    assert(exit.size()==10 && exit[0].kind==K::ppdu_enable);
    for(auto op:exit)assert(op.kind!=K::resume_scheduler);
    auto full=archer::channel1_reset_exit(context,true);assert(full.back().kind==K::resume_scheduler);
    auto slots=archer::channel1_fresh_mcc_slots(true,true);assert(slots[0]==0 && slots[1]==0);
    rejects([]{archer::channel1_fresh_mcc_slots(false,true);});
    rejects([]{archer::channel1_fresh_mcc_slots(true,false);});
    auto post=archer::channel1_post_rf_plan(context,{0,2},{0xabcde,0x54321});
    assert(post.operations.size()==12 && post.selected_table[1]==2 && post.rf18==1);
    assert(post.operations[3].kind==K::full_rf_write && post.operations[3].address==1);
    assert(post.operations[3].value==((0xabcde&~0xe0000u)|0x80000));
    assert(post.operations[9].path==1 && post.operations[9].value==((0x54321&~0xe0000u)|0xc0000));
    assert(post.operations[10].value==0 && post.operations[11].value==0x20);
    for(const auto& op:post.operations)assert((op.value&~op.mask)==0);
    rejects([&]{archer::channel1_post_rf_plan(context,{0,3},{0,0});});
    rejects([&]{archer::channel1_post_rf_plan(context,{0,0},{0xffffffff,0});});
    std::cout<<"Channel reset bracket and post-RF MCC plans: OK\n";
}
