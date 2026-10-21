#include "archer/bb_postinit.hpp"
#include <cassert>
#include <map>
#include <tuple>
#include <iostream>
struct Fake : archer::RegisterBus {
    std::map<unsigned,unsigned> registers;
    std::vector<std::tuple<unsigned,unsigned,unsigned>> writes;
    unsigned reads=0, polls=0, ready_after=1;
    bool emulate_poll=false;
    unsigned read(unsigned a,unsigned width) override {
        assert(width==4); ++reads;
        if(emulate_poll && a==0x28080)return ++polls>=ready_after?4:0;
        return registers.count(a)?registers[a]:0xffffffff;
    }
    void write(unsigned a,unsigned width,unsigned v) override {
        assert(width==4);registers[a]=v;writes.emplace_back(a,width,v);
    }
    void pause() override {assert(false);}
};
template<class F> void rejects(F f) {
    bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);
}
int main() {
    using namespace archer;
    Fake bus;
    const auto plan=rtl8922a_bb_postinit_plan();
    apply_bb_init_plan(bus,plan,{});
    assert(bus.registers[0x84]==0xffffffff); // preservation of unrelated FEN bits
    assert(bus.registers[0x26284]==0xfff7ffff);
    assert(bus.registers[0x2627c]==0xfff803ff);
    assert(bus.registers[0x26b6c]==0xaff83fe0);
    assert(bus.registers[0x26bfc]==0xd7ffffff);
    assert(bus.registers[0x26c00]==0xfffffaaa);
    assert(bus.registers[0x26c04]==0xff577fff);
    assert(bus.registers[0x26bf8]==0x01e0c000);
    assert(bus.registers[0x26bf4]==0xe0ffffff);
    assert(bus.registers[0x26b70]==0xfffffa00);
    assert(std::get<0>(bus.writes[bus.writes.size()-2])==0x20cbc);
    assert(std::get<2>(bus.writes[bus.writes.size()-2])==0xfff7ffff);
    assert(std::get<2>(bus.writes.back())==0xffffffff);
    Fake dual;dual.registers[0x84]=0;
    apply_bb_init_plan(dual,rtl8922a_bb_postinit_plan(1,true),{});
    assert(dual.registers[0x84]==0x100); // PHY1 does not set BOOT_RDY1
    assert(dual.registers[0x27bf8]==0x01e0c000);
    assert(dual.registers.count(0x21cbc));
    rejects([&]{rtl8922a_bb_postinit_plan(2);});
    rejects([&]{rtl8922a_bb_postinit_plan(1,false);});
    Fake pre;
    apply_bb_init_plan(pre,be_nctl_preinit_plan(),{});
    assert(pre.registers[0x21008]==0xfe0fffff);
    assert(pre.registers[0x20c7c]==0xff3fffff);
    assert(!pre.registers.count(0x21c7c));
    assert(be_nctl_preinit_plan(true).size()==be_nctl_preinit_plan(false).size()+2);
    std::vector<std::pair<unsigned,unsigned>> pairs{{0x8000,8},{0xf9,0},{0xfa,0},{0xfb,0},
        {0xfc,0},{0xfd,0},{0xfe,0},{0x8080,4},{0x9000,0xbabecafe}};
    auto table=selected_rf_nctl_plan(pairs);
    assert(table.size()==8);
    Fake tbl;std::vector<unsigned> delays;
    apply_bb_init_plan(tbl,table,[&](unsigned us){delays.push_back(us);});
    assert((delays==std::vector<unsigned>{1,5,50,1000,5000,50000}));
    assert(tbl.reads==0 && tbl.writes.size()==2 && tbl.registers[0x28080]==4);
    // The whole input is validated before any effect, including a bad trailing opcode.
    auto bad=table;bad.push_back({0x30000,0xffffffff,0,0});
    Fake untouched;
    rejects([&]{apply_bb_init_plan(untouched,bad,{});});
    assert(untouched.reads==0 && untouched.writes.empty());
    rejects([&]{apply_bb_init_plan(untouched,table,{});});
    assert(untouched.writes.empty());
    rejects([&]{selected_rf_nctl_plan({{0x8000,8},{0x80000000,1}});});
    rejects([&]{selected_rf_nctl_plan({{0x8001,8}});});
    rejects([&]{apply_bb_init_plan(untouched,{{0x20000,1,2,0}},{});});
    Fake poll;poll.emulate_poll=true;poll.ready_after=3;
    unsigned elapsed=0;
    assert(check_be_nctl(poll,[&](unsigned us){elapsed+=us;},3)==3);
    assert(elapsed==30 && poll.writes.size()==4);
    Fake timeout;timeout.emulate_poll=true;timeout.ready_after=4;
    rejects([&]{check_be_nctl(timeout,[](unsigned){},3);});
    assert(timeout.polls==3 && timeout.writes.size()==4);
    rejects([&]{check_be_nctl(untouched,[](unsigned){},10001);});
    assert(untouched.writes.empty());
    Fake inaccessible;inaccessible.registers[0x20000]=0xdeadbeef;
    rejects([&]{apply_bb_init_plan(inaccessible,{{0x20000,4,4,0}},{});});
    assert(inaccessible.writes.empty());
    std::cout<<"BB postinit/NCTL tests passed\n";
}
