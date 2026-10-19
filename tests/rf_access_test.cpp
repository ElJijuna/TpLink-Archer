#include "archer/rf_access.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}assert(caught);}
int main(){
    assert(archer::be_bb_register(0x8080)==0x28080);
    auto analog=archer::be_rf_write(0,0x55,0xabcde);
    assert(!analog.direct && analog.status_address==0x22c24 && analog.status_busy_mask==(1u<<29));
    assert(analog.target_address==0x22ae0 && analog.value==0x0abcde55 && analog.mask==0xffffffff);
    auto direct=archer::be_rf_write(1,0x100ef,0x12345);
    assert(direct.direct && direct.target_address==0x2f3bc && direct.value==0x12345 && direct.mask==0xfffff);
    rejects([]{archer::be_rf_write(2,0,0);});
    rejects([]{archer::be_rf_write(0,0x100,0);});
    rejects([]{archer::be_rf_write(0,0,0x100000);});
    rejects([]{archer::be_bb_register(3);});
    std::cout<<"BE BB mapping and RF-v2/HWSI encoding: OK\n";
}
