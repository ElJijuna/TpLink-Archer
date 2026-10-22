#pragma once
#include "archer/channel_bus.hpp"
#include <chrono>
#include <thread>
namespace archer {
// Debug read requests only. Never issue buffer allocation, dequeue or reset.
inline unsigned ple_debug_read(RegisterBus& bus,unsigned target,unsigned address){
    if(!((target==7&&address==0)||(target==1&&address==6)||
         (target==6&&address<64&&(address&1))))
        throw std::invalid_argument("PLE diagnostic selector outside fixed scope");
    const auto saved=channel_bus_read(bus,0x9110);
    if(saved&0x80000000)throw std::runtime_error("PLE debug interface already active");
    std::exception_ptr failure;unsigned result=0;
    try{
        bus.write(0x9110,4,0x80000000|(target<<16)|address);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(2);
        bool done=false;
        for(unsigned i=0;i<1000;++i){
            if(!(channel_bus_read(bus,0x9110)&0x80000000)){done=true;break;}
            if(std::chrono::steady_clock::now()>=deadline)break;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        if(!done)throw std::runtime_error("PLE debug request timeout; no retry");
        result=channel_bus_read(bus,0x9114);
    }catch(...){failure=std::current_exception();}
    if(channel_bus_read(bus,0x9110)&0x80000000)
        throw std::runtime_error("PLE debug request pending; selector left untouched");
    bus.write(0x9110,4,saved);
    if(channel_bus_read(bus,0x9110)!=saved)throw std::runtime_error("PLE debug selector restoration failed");
    if(failure)std::rethrow_exception(failure);
    return result;
}
}
