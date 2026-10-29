#include "archer/led_window.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus : archer::RegisterBus {
    std::map<unsigned,unsigned> regs{{0x2d4,0xa6},{0x61,0x55},{0x62,0xa501}};
    unsigned writes=0,fail=0;
    unsigned read(unsigned r,unsigned) override {return regs.at(r);}
    void write(unsigned r,unsigned,unsigned v) override {
        regs[r]=v;
        if(++writes==fail)throw std::runtime_error("injected failure after write");
    }
    void pause() override {}
};
int main() {
    for(unsigned failure=0;failure<=4;++failure) {
        Bus b;const auto before=b.regs;b.fail=failure;bool visited=false,caught=false;
        try { archer::with_gpio8_led(b,[&] {
            visited=true;assert(b.regs.at(0x2d4)==0xaf);
            assert(b.regs.at(0x61)==0x54);assert(b.regs.at(0x62)==0xa501);
            // A change to unowned bits must survive restoration.
            b.regs[0x61]|=0x80;
        }); } catch(const std::runtime_error&) {caught=true;}
        assert(caught==(failure!=0));assert(visited==(failure==0));
        auto expected=before;if(visited)expected[0x61]|=0x80;
        assert(b.regs==expected);
    }
    Bus b;const auto before=b.regs;bool caught=false;
    try {archer::with_gpio8_led(b,[]{throw std::runtime_error("observer failed");});}
    catch(const std::runtime_error&){caught=true;}
    assert(caught&&b.regs==before);
    std::cout<<"LED masked restoration and partial-write recovery: OK\n";
}
