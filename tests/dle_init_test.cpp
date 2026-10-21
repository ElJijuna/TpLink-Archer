#include "archer/dle_init.hpp"
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
constexpr unsigned engines=(1u<<26)|(1u<<23);
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> regs{{0xfc,0x71},{0xf0,0x1000},{0x8400,0x60000001},
        {0x8404,0x10},{0x8c08,0x80000080},{0x9008,0x80000080},{0x8d00,3},{0x9100,3}};
    std::vector<unsigned> writes,reads;
    unsigned fail=0, drop=0, wde_pending=0; bool broken_cleanup=false;
    unsigned read(unsigned a,unsigned w) override {assert(w==4);reads.push_back(a);if(a==0x8d00 && wde_pending){--wde_pending;return 0;}return regs[a];}
    void write(unsigned a,unsigned w,unsigned v) override {
        assert(w==4);writes.push_back(a);
        if(a==fail || (broken_cleanup && a==0x8400 && writes.size()>2 && !(v&engines)))
            throw std::runtime_error("injected transfer failure");
        if(a!=drop)regs[a]=v;
    }
    void pause() override {assert(false);}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    archer::DleColdStartEvidence evidence{true,true,true,true};
    Bus ok; unsigned delays=0;
    archer::initialize_usb2_scc_dle(ok,evidence,[&]{++delays;});
    assert(delays==0 && ok.writes.size()==23);
    assert(ok.writes[0]==0x8400 && ok.writes[1]==0x8404 && ok.writes[2]==0x8c08 && ok.writes[3]==0x9008);
    assert(ok.writes.back()==0x8400 && (ok.regs[0x8400]&engines)==engines);
    assert(ok.regs[0x8c08]==(0x80000080|(508u<<16)));
    assert(ok.regs[0x9008]==(0x80000080|(2059u<<16)|0x402));
    assert(ok.reads[ok.reads.size()-2]==0x8d00 && ok.reads.back()==0x9100);
    Bus all_clocks;all_clocks.regs[0x8404]=0xffffffff;
    archer::initialize_usb2_scc_dle(all_clocks,evidence,[]{});
    assert(all_clocks.regs[0x8404]==0xffffffff);
    Bus delayed;delayed.wde_pending=3;unsigned delayed_ticks=0;
    archer::initialize_usb2_scc_dle(delayed,evidence,[&]{++delayed_ticks;});
    assert(delayed_ticks==3);
    Bus latch;latch.drop=0x9008;
    rejects([&]{archer::initialize_usb2_scc_dle(latch,evidence,[]{});});
    assert(!(latch.regs[0x8400]&engines));
    Bus live;live.regs[0x1e0]=3u<<26;
    rejects([&]{archer::initialize_usb2_scc_dle(live,evidence,[]{});});assert(live.writes.empty());
    Bus unproven;rejects([&]{archer::initialize_usb2_scc_dle(unproven,{},[]{});});assert(unproven.reads.empty());
    for(auto address:{0x8d00u,0x9100u}) {
        Bus timeout;timeout.regs[address]=1;unsigned ticks=0;
        rejects([&]{archer::initialize_usb2_scc_dle(timeout,evidence,[&]{++ticks;});});
        assert(ticks==2000 && !(timeout.regs[0x8400]&engines));
    }
    Bus partial;partial.fail=0x9048;
    rejects([&]{archer::initialize_usb2_scc_dle(partial,evidence,[]{});});
    assert(!(partial.regs[0x8400]&engines) && partial.writes.back()==0x8400);
    Bus inaccessible;inaccessible.regs[0x8404]=0xeaeaeaea;
    rejects([&]{archer::initialize_usb2_scc_dle(inaccessible,evidence,[]{});});
    assert(!(inaccessible.regs[0x8400]&engines));
    Bus cleanup;cleanup.fail=0x9048;cleanup.broken_cleanup=true;
    rejects([&]{archer::initialize_usb2_scc_dle(cleanup,evidence,[]{});});
    std::cout<<"Cold USB2 SCC DLE ordering, guards, timeouts and failure handling: OK\n";
}
