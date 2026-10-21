#pragma once
#include "channel_transition.hpp"
#include "rf_transport.hpp"
namespace archer {
inline unsigned channel_bus_read(RegisterBus& bus,unsigned a,unsigned width=4) {
    const auto value=bus.read(a,width);
    if(value==0xdeadbeef || value==0xeaeaeaea)
        throw std::runtime_error("Channel control inaccessible");
    return value;
}
inline void channel_bus_write(RegisterBus& bus,unsigned a,unsigned mask,unsigned value,unsigned width=4) {
    if(!mask || (value&~mask) || (width!=1 && width!=4) || (width==1 && mask>255))
        throw std::invalid_argument("Invalid channel masked write");
    bus.write(a,width,(channel_bus_read(bus,a,width)&~mask)|value);
    if((channel_bus_read(bus,a,width)&mask)!=value)
        throw std::runtime_error("Channel field did not latch; keep scheduler paused");
}
inline unsigned pause_cmac0_scheduler(RegisterBus& bus) {
    if((channel_bus_read(bus,0xfc)&255)!=0x71 || !(channel_bus_read(bus,0x10000)&0x40000000))
        throw std::runtime_error("CMAC0 scheduler prerequisites absent");
    const auto saved=channel_bus_read(bus,0x10398);
    channel_bus_write(bus,0x10398,0x3ffff,0);
    return saved;
}
inline void resume_cmac0_scheduler(RegisterBus& bus,unsigned saved) {
    channel_bus_write(bus,0x10398,0x3ffff,saved&0x3ffff);
}
inline void set_cmac0_ppdu(RegisterBus& bus,bool enabled) {
    if(!(channel_bus_read(bus,0x10000)&0x40000000))throw std::runtime_error("CMAC0 not enabled");
    if(!enabled)channel_bus_write(bus,0x11440,1,0);
    else {
        channel_bus_write(bus,0x9c30,255,3);
        bus.write(0x11440,4,0x6d);
        if((channel_bus_read(bus,0x11440)&0x6d)!=0x6d)
            throw std::runtime_error("PPDU report configuration did not latch");
    }
}
// Source tests RF mode !=2; this means "not TX", not proof of RX sensitivity.
template<class ReadRf,class Delay,class Expired>
inline void wait_rf_not_transmitting(ReadRf read_rf,Delay delay_us,Expired expired) {
    for(unsigned path=0;path<2;++path) {
        bool ready=false;
        for(unsigned i=0;i<=2500;++i) {
            const auto value=read_rf(path,0);
            if(value>0xfffff)throw std::runtime_error("RF mode read invalid");
            if(((value>>16)&15)!=2){ready=true;break;}
            if(expired())break;
            delay_us(2);
        }
        if(!ready)throw std::runtime_error("RF remains in TX mode; no calibration submitted");
    }
}
// Stateful coordinator for the already validated pure reset/post-RF plans.
// Never resumes scheduler implicitly on failure or destruction.
class ChannelBus {
    RegisterBus& bus;
    BeRfTransport rf;
    std::function<void(unsigned)> delay_us;
    std::optional<unsigned> saved_scheduler;
public:
    ChannelBus(RegisterBus& b,std::function<void(unsigned)> delay):bus(b),rf(b),delay_us(delay) {
        if(!delay)throw std::invalid_argument("Channel timing callback required");
    }
    bool paused() const{return saved_scheduler.has_value();}
    void execute(const std::vector<ChannelTransitionOp>& plan) {
        using K=ChannelTransitionKind;
        for(const auto& op:plan) {
            switch(op.kind) {
            case K::pause_scheduler:
                if(saved_scheduler)throw std::runtime_error("Nested scheduler ownership");
                saved_scheduler=pause_cmac0_scheduler(bus);break;
            case K::resume_scheduler:
                if(!saved_scheduler)throw std::runtime_error("No scheduler snapshot");
                resume_cmac0_scheduler(bus,*saved_scheduler);saved_scheduler.reset();break;
            case K::ppdu_disable:case K::ppdu_enable:
                if(!saved_scheduler)throw std::runtime_error("Scheduler must be paused before PPDU transition");
                set_cmac0_ppdu(bus,op.kind==K::ppdu_enable);break;
            case K::delay_us:
                if(op.value>50000)throw std::invalid_argument("Channel delay bound exceeded");
                delay_us(op.value);break;
            case K::masked_bb_write:
                if(!saved_scheduler || op.address<0x20000 || op.address>0x2fffc || op.address%4)
                    throw std::runtime_error("BB transition requires paused scheduler and valid address");
                channel_bus_write(bus,op.address,op.mask,op.value);break;
            case K::full_rf_write:
                if(!saved_scheduler)throw std::runtime_error("RF transition requires paused scheduler");
                rf.submit_full_write(op.path,op.address,op.value);
                if((rf.read_full(op.path,op.address)&op.mask)!=(op.value&op.mask))
                    throw std::runtime_error("RF selector did not latch");
                break;
            }
        }
    }
};
} // namespace archer
