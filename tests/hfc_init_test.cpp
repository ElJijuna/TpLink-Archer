#include "archer/hfc_init.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> regs{{0xfc,0x71},{0xf0,0x1000},{0x8400,0x60000000},
        {0x8d00,3},{0x9100,3},{0xb700,0x80003000},{0xb7a0,492},{0xb78c,492}};
    unsigned writes=0,fail=0,drop=0;bool normalize_h2c_condition=false;
    unsigned read(unsigned a,unsigned w)override{assert(w==4);return regs[a];}
    void write(unsigned a,unsigned w,unsigned v)override{
        assert(w==4);++writes;if(a==fail)throw std::runtime_error("injected transfer failure");
        if(a!=drop)regs[a]=normalize_h2c_condition && a==0xb700 ? v&~0xc00u : v;
    }
    void pause()override{assert(false);}
};
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
int main(){
    auto profile=archer::usb2_scc_hfc_profile();
    auto plan=archer::usb2_scc_hfc_plan(profile);assert(plan.size()==21);
    assert(plan[1].value==0x00de0036 && plan[2].value==0);
    Bus ok;ok.regs[0xb750]=0x00360003;unsigned delay=0;
    auto info=archer::initialize_usb2_scc_hfc(ok,profile,true,true,[&](unsigned us){delay+=us;});
    assert(ok.writes==20 && delay==10 && ok.regs[0xb700]==0x8000351b);
    assert(info.channel_available[0]==54 && info.channel_used[0]==3 && info.public_available==492);
    assert(info.group0_available==492 && info.group1_available==0);
    // Observed stage32: requested CH12 condition1 reads back0. Source
    // accepts and adopts that effective condition; public accounting still applies.
    Bus normalized;normalized.normalize_h2c_condition=true;
    auto effective=archer::initialize_usb2_scc_hfc(normalized,profile,true,true,[](unsigned){});
    assert(effective.h2c_full_condition_requested==1 && effective.h2c_full_condition_effective==0);
    assert(effective.control_profile_difference==0x400 && (effective.control&9)==9);
    Bus missing;
    rejects([&]{archer::initialize_usb2_scc_hfc(missing,profile,false,true,[](unsigned){});});assert(!missing.writes);
    auto wrong=profile;wrong.mode=0;rejects([&]{archer::usb2_scc_hfc_plan(wrong);});
    wrong=profile;wrong.minimum[0]=0;rejects([&]{archer::usb2_scc_hfc_plan(wrong);});
    wrong=profile;wrong.dma_channel_skip_mask=1;rejects([&]{archer::usb2_scc_hfc_plan(wrong);});
    Bus account;account.regs[0xb7a0]=491;
    rejects([&]{archer::initialize_usb2_scc_hfc(account,profile,true,true,[](unsigned){});});assert(!(account.regs[0xb700]&9));
    Bus used;used.regs[0xb79c]=1;used.regs[0xb7a0]=491;
    auto busy=archer::initialize_usb2_scc_hfc(used,profile,true,true,[](unsigned){});assert(busy.group0_used==1);
    Bus transfer;transfer.fail=0xb794;
    rejects([&]{archer::initialize_usb2_scc_hfc(transfer,profile,true,true,[](unsigned){});});assert(!(transfer.regs[0xb700]&9));
    Bus latch;latch.drop=0xb704;
    rejects([&]{archer::initialize_usb2_scc_hfc(latch,profile,true,true,[](unsigned){});});assert(!(latch.regs[0xb700]&9));
    Bus unavailable;unavailable.regs[0xb7a0]=0xeaeaeaea;
    rejects([&]{archer::initialize_usb2_scc_hfc(unavailable,profile,true,true,[](unsigned){});});assert(!(unavailable.regs[0xb700]&9));
    std::cout<<"USB2 SCC HFC exact profile, accounting and bounded failure handling: OK\n";
}
