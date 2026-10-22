#pragma once
#include "archer/channel_bus.hpp"
#include <vector>
namespace archer {
inline unsigned rtl8922a_physts_address(unsigned page){
    if(page>16 || page==9)throw std::invalid_argument("Invalid PHY status page");
    if(page==16)return 0x20788;
    return 0x2073c+4*(page>9?page-1:page);
}
inline unsigned rtl8922a_physts_value(unsigned page,unsigned old){
    (void)rtl8922a_physts_address(page);
    if(page==6 || page==7)return old|(1u<<13);
    if(page==10)return old|(1u<<13)|2;
    if(page>=11){old&=~0xf0u;if(page==11)old|=2;else if(page>=13)old|=1u<<20;}
    return old;
}
// PHY0, non-monitor RTL8922A/gen_be. No hdr2 on this chip. Journal source
// bitmap writes and disable fail/break reports; restore even after I/O failure.
inline void with_physts_window(RegisterBus& bus,const std::function<void()>& receive){
    if((channel_bus_read(bus,0xfc)&255)!=0x71 ||
       (channel_bus_read(bus,0x10398)&0x3ffff) || !(channel_bus_read(bus,0x10000)&0x40000000))
        throw std::runtime_error("PHY status window requires paused RTL8922A CMAC0");
    struct Write{unsigned address,mask,saved,value;};
    std::vector<Write> plan;
    const auto setting=channel_bus_read(bus,0x20738);
    plan.push_back({0x20738,12,setting,12});
    for(unsigned page=0;page<=16;++page){
        if(page==9)continue;
        const auto address=rtl8922a_physts_address(page),old=channel_bus_read(bus,address);
        plan.push_back({address,0xffffffff,old,rtl8922a_physts_value(page,old)});
    }
    unsigned attempted=0;std::exception_ptr failure;
    try{
        for(const auto& w:plan){++attempted;channel_bus_write(bus,w.address,w.mask,w.value);}
        receive();
    }catch(...){failure=std::current_exception();}
    std::string errors;
    while(attempted){const auto& w=plan[--attempted];
        try{channel_bus_write(bus,w.address,w.mask,w.saved&w.mask);}
        catch(const std::exception& e){errors+=std::string(e.what())+"; ";}
    }
    if(!errors.empty())throw std::runtime_error("PHY status restoration failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
