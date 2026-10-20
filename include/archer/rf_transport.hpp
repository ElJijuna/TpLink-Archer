#pragma once
#include "archer/register_mailbox.hpp"
#include "archer/rf_access.hpp"
namespace archer {
// Full RF access for a caller that has already initialized BB/MAC. No power/reset writes.
class BeRfTransport {
    RegisterBus& bus_;
    std::uint32_t get(std::uint32_t address) {
        const auto value=bus_.read(address,4);
        if(value==0xeaeaeaea || value==0xdeadbeef)
            throw std::runtime_error("BB/RF register inaccessible");
        return value;
    }
    void update(std::uint32_t address,std::uint32_t mask,std::uint32_t value) {
        bus_.write(address,4,(get(address)&~mask)|(value&mask));
    }
    void ready() {
        if((get(0xfc)&255)!=0x71 || ((get(0x3f0)>>16)&0x3ff)!=0x220 ||
           (get(0x74)&0x90000000)!=0x90000000 || (get(0x84)&0x33)!=3 ||
           (get(0x842c)&0xffff)!=0x7ff9)
            throw std::runtime_error("RF prerequisites missing: active WLAN IO and initialized BB required");
    }
    void idle(std::uint32_t status) {
        for(unsigned attempt=0;attempt<6;++attempt) {
            if(!(get(status)&(1u<<29)))return;
            bus_.pause();
        }
        throw std::runtime_error("HWSI busy timeout; no RF command submitted");
    }
public:
    explicit BeRfTransport(RegisterBus& bus):bus_(bus){}
    std::uint32_t read_full(unsigned path,std::uint32_t address) {
        const auto plan=be_rf_write(path,address,0); // Validate fields before I/O.
        ready();
        if(plan.direct)return get(plan.target_address)&0xfffff;
        const auto control=be_bb_register(path==0?0x2adc:0x2bdc);
        if(get(control)&3)throw std::runtime_error("HWSI command latch busy");
        idle(plan.status_address);
        std::exception_ptr failure;
        std::uint32_t result=0;
        try {
            update(control,7,1);
            update(control,0xff0,address<<4);
            update(control,4,4);
            bool done=false;
            for(unsigned attempt=0;attempt<6;++attempt) {
                const auto status=get(plan.status_address);
                if(status&(1u<<31)){result=status&0xfffff;done=true;break;}
                bus_.pause();
            }
            if(!done)throw std::runtime_error("HWSI read completion timeout");
        } catch(...){failure=std::current_exception();}
        try {update(control,3,0);if(get(control)&3)throw std::runtime_error("Poll bits did not clear");}
        catch(const std::exception& error){throw std::runtime_error(std::string("HWSI cleanup failed: ")+error.what());}
        if(failure)std::rethrow_exception(failure);
        return result;
    }
    // Submission only; no claim that a write or a table has been calibrated/applied.
    void submit_full_write(unsigned path,std::uint32_t address,std::uint32_t value) {
        const auto plan=be_rf_write(path,address,value);
        ready();
        if(plan.direct) update(plan.target_address,plan.mask,plan.value);
        else {idle(plan.status_address);bus_.write(plan.target_address,4,plan.value);}
    }
};
}
