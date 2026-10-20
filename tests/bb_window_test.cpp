#include "archer/bb_window.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0xfc,0x371},{0xf0,0x1000},{0x3f0,0x2200000},
        {0x74,0x901004fd},{0x84,4},{0x842c,0}};
    unsigned writes=0;bool fail_restore=false,fail_config=false;
    unsigned read(unsigned a,unsigned)override{return r[a];}
    void write(unsigned a,unsigned,unsigned v)override{
        ++writes;
        if(a==0x84 && !(v&3) && fail_restore)throw std::runtime_error("Injected reset restore error");
        if(a==0x842c && v && fail_config)throw std::runtime_error("Injected DMAC config failure");
        r[a]=v;
    }
    void pause()override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    Bus ok;bool called=false;
    archer::with_bb_diagnostic_window(ok,[&]{called=true;assert(ok.r[0x84]==7 && ok.r[0x842c]==0x7ff97ff9);ok.r[0x84]|=8;});
    assert(called && ok.r[0x84]==12 && ok.r[0x842c]==0);
    Bus failed;rejects([&]{archer::with_bb_diagnostic_window(failed,[]{throw std::runtime_error("callback");});});
    assert(failed.r[0x84]==4 && failed.r[0x842c]==0);
    Bus transfer;transfer.fail_config=true;rejects([&]{archer::with_bb_diagnostic_window(transfer,[]{});});
    assert(transfer.r[0x84]==4 && transfer.r[0x842c]==0);
    Bus cleanup;cleanup.fail_restore=true;rejects([&]{archer::with_bb_diagnostic_window(cleanup,[]{});});
    assert(cleanup.r[0x842c]==0);
    Bus busy;busy.r[0x84]=7;rejects([&]{archer::with_bb_diagnostic_window(busy,[]{});});assert(!busy.writes);
    Bus unknown;unknown.r[0x842c]=1;rejects([&]{archer::with_bb_diagnostic_window(unknown,[]{});});assert(!unknown.writes);
    std::cout<<"Temporary BB window prerequisites and restoration: OK\n";
}
