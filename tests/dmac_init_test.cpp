#include "archer/dmac_init.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> regs{{0xfc,0x71},{0xf0,0x1000},{0x8400,0x60000000},
        {0x8d00,3},{0x9100,3},{0xe000,0x40000007},{0xa310,0x80000000},{0xa114,0x80000000},
        {0x9b00,0x10},{0x8938,0x12340000},{0x9f10,0x7000ffc0}};
    unsigned writes=0, fail=0, drop=0;bool normalize_cut=false;
    unsigned read(unsigned a,unsigned w)override{return regs[a]&(w==1?255u:0xffffffffu);}
    void write(unsigned a,unsigned w,unsigned v)override{
        ++writes;if(a==fail)throw std::runtime_error("injected USB failure");
        if(a!=drop)regs[a]=(regs[a]&(w==1?0xffffff00:0))|v;
        if(a==0x9c94 && normalize_cut)regs[a]&=~0x01000000u;
    }
    void pause()override{assert(false);}
};
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
int main(){
    archer::DmacRemainderInputs inputs{true,true,true,archer::DmacReservedMpduInfo{2059,256,1,18}};
    const auto plan=archer::usb2_scc_dmac_remainder_plan(inputs);assert(plan.size()==17);
    Bus shared;shared.regs[0xe000]=0x20000008;
    archer::enable_8922a_shared_cmac(shared,true);
    assert(shared.regs[0xe000]==0x6000000f && shared.writes==1);
    Bus no_owner;rejects([&]{archer::enable_8922a_shared_cmac(no_owner,false);});assert(!no_owner.writes);
    Bus shared_latch;shared_latch.regs[0xe000]=0;shared_latch.drop=0xe000;
    rejects([&]{archer::enable_8922a_shared_cmac(shared_latch,true);});
    Bus missing_shared;missing_shared.regs[0xe000]=0;
    rejects([&]{archer::initialize_usb2_scc_dmac_remainder(missing_shared,inputs,[]{});});
    assert(!missing_shared.writes);
    Bus ok;unsigned delays=0;
    archer::initialize_usb2_scc_dmac_remainder(ok,inputs,[&]{++delays;});
    assert(ok.writes==15 && delays==0 && ok.regs[0x9b00]==0x17);
    assert(ok.regs[0x8938]==0x12340055 && ok.regs[0x9c94]==0x210e05f0);
    assert((ok.regs[0x9f10]&0x8fff003f)==0x880b0012);
    assert((ok.regs[0x9f10]&~0x8fff003fu)==(0x7000ffc0&~0x8fff003fu));
    Bus normalized;normalized.normalize_cut=true;
    assert(archer::initialize_usb2_scc_dmac_remainder(normalized,inputs,[]{})==0x200e05f0);
    Bus delayed;delayed.regs[0xa310]=0;unsigned scheduler_ticks=0;
    archer::initialize_usb2_scc_dmac_remainder(delayed,inputs,[&]{
        if(++scheduler_ticks==3)delayed.regs[0xa310]|=0x80000000;
    });
    assert(scheduler_ticks==3 && (delayed.regs[0xe010]&1));
    Bus missing;auto bad=inputs;bad.reserved_mpdu.reset();
    rejects([&]{archer::initialize_usb2_scc_dmac_remainder(missing,bad,[]{});});assert(!missing.writes);
    bad=inputs;bad.usb2_scc_hfc_initialized=false;
    rejects([&]{archer::usb2_scc_dmac_remainder_plan(bad);});
    bad=inputs;bad.reserved_mpdu->ple_free_pages=1963;
    rejects([&]{archer::usb2_scc_dmac_remainder_plan(bad);});
    for(auto address:{0xa310u,0xa114u}){
        Bus timeout;timeout.regs[address]&=0x7fffffff;unsigned ticks=0;
        rejects([&]{archer::initialize_usb2_scc_dmac_remainder(timeout,inputs,[&]{++ticks;});});
        assert(ticks==(address==0xa310?2000:1000));
        assert(!(timeout.regs[0xe010]&1));
    }
    Bus transfer;transfer.fail=0x9c94;
    rejects([&]{archer::initialize_usb2_scc_dmac_remainder(transfer,inputs,[]{});});assert(!(transfer.regs[0xe010]&1));
    Bus ignored;ignored.drop=0x9d04;
    rejects([&]{archer::initialize_usb2_scc_dmac_remainder(ignored,inputs,[]{});});assert(!(ignored.regs[0xe010]&1));
    Bus sentinel;sentinel.regs[0x8400]=0xeaeaeaea;
    rejects([&]{archer::initialize_usb2_scc_dmac_remainder(sentinel,inputs,[]{});});assert(!sentinel.writes);
    std::cout<<"DMAC remainder metadata, ordering, preservation and bounded failures: OK\n";
}
