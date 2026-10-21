#pragma once
#include "register_mailbox.hpp"
#include "rf_access.hpp"
#include <vector>
#include <utility>
namespace archer {
struct BbInitOperation {
    std::uint32_t address, mask, value; // physical address; value is already positioned
    unsigned delay_microseconds = 0; // delay iff address/mask/value all zero
};
inline unsigned bb_be_phy_offset(unsigned logical) {
    const auto page = logical >> 8;
    return ((page>=4 && page<=15) || (page>=0x20 && page<=0x2b) ||
            (page>=0x40 && page<=0x4f) || (page>=0x60 && page<=0x6f) ||
            (page>=0xe4 && page<=0xe5) || (page>=0xe8 && page<=0xed)) ? 0x1000 : 0;
}
inline std::vector<BbInitOperation> rtl8922a_bb_postinit_plan(unsigned phy=0, bool dbcc=false) {
    if (phy>1 || (phy==1 && !dbcc)) throw std::invalid_argument("Invalid BB PHY/DBCC context");
    std::vector<BbInitOperation> plan;
    auto physical=[&](unsigned a,unsigned m,unsigned v){ plan.push_back({a,m,v,0}); };
    auto reg=[&](unsigned a,unsigned m,unsigned v){physical(be_bb_register(a),m,v);};
    auto both=[&](unsigned a,unsigned m,unsigned v){reg(a,m,v); if(dbcc)reg(a+bb_be_phy_offset(a),m,v);};
    if(phy==0)physical(0x84,4,4); // Host writes BOOT_RDY0; not an MCU liveness observation.
    physical(0x84,phy==0?1:0x100,phy==0?1:0x100);
    reg(0,4,4); reg(0x6284,0x80000,0);
    both(0x627c,0xffc00,0x80000);
    both(0x6b6c,0xf0000000,0xa0000000);
    both(0x6bfc,0x78000000,0x50000000);
    both(0x6c00,0xfff,0xaaa);
    both(0x6bfc,0x80000000,0x80000000);
    both(0x6c00,0x80000000,0x80000000);
    both(0x6c04,0x8000,0); both(0x6c04,0x80000,0);
    both(0x6c04,0x200000,0); both(0x6c04,0x800000,0);
    both(0x6c08,2,2); both(0x6c10,0x2000000,0x2000000);
    both(0x6c10,0x100000,0x100000); both(0x6bf8,0xff000000,0x1000000);
    both(0x6bf4,0xff000000,0xe0000000); both(0x6bf8,0xffffff,0xe0c000);
    both(0x6b6c,0x3fff,0x3fe0); both(0x6b6c,0xfffc000,0xff80000);
    both(0x6b70,0x7ff,0x200);
    const unsigned udp=0xcbc+(phy?bb_be_phy_offset(0xcbc):0);
    reg(udp,0x80000,0); reg(udp,0x80000,0x80000);
    return plan;
}
inline std::vector<BbInitOperation> be_nctl_preinit_plan(bool dbcc=false) {
    std::vector<BbInitOperation> p;
    auto add=[&](unsigned a,unsigned m,unsigned v){p.push_back({be_bb_register(a),m,v,0});};
    add(0xe464,0x18000000,0x18000000); add(0xe564,0x18000000,0x18000000);
    add(0x2ab8,0x10000000,0x10000000); add(0x1008,0x1ffffff,0xfffff);
    add(0xc60,3,3); add(0xc6c,1,1); add(0xe4ac,0x8000000,0x8000000);
    add(0xe5ac,0x8000000,0x8000000); add(0xc7c,0xe00000,0x200000);
    if(dbcc){add(0x1c6c,1,1);add(0x1c7c,0xe00000,0x200000);}
    add(0xc70,0x200,0x200); add(0xc70,0x60,0x60);
    return p;
}
inline void validate_bb_init_plan(const std::vector<BbInitOperation>& plan) {
    if(plan.empty() || plan.size()>8192)throw std::invalid_argument("Invalid BB initialization plan size");
    for(const auto& op:plan) {
        if(op.delay_microseconds) {
            if(op.address || op.mask || op.value || op.delay_microseconds>50000)
                throw std::invalid_argument("Invalid BB delay operation");
        } else if((op.address!=0x84 && (op.address<0x20000 || op.address>0x2fffc || op.address%4)) ||
                  !op.mask || (op.value&~op.mask))
            throw std::invalid_argument("Invalid BB masked write");
    }
}
inline unsigned bb_init_read(RegisterBus& bus,unsigned address) {
    const auto value=bus.read(address,4);
    if(value==0xdeadbeef || value==0xeaeaeaea)throw std::runtime_error("BB initialization register inaccessible");
    return value;
}
inline void apply_bb_init_plan(RegisterBus& bus,const std::vector<BbInitOperation>& plan,
                              const std::function<void(unsigned)>& delay_us) {
    validate_bb_init_plan(plan); // Entire plan validated before the first read/write/delay.
    for(const auto& op:plan)if(op.delay_microseconds && !delay_us)
        throw std::invalid_argument("BB plan requires explicit microsecond delay callback");
    for(const auto& op:plan) {
        if(op.delay_microseconds)delay_us(op.delay_microseconds);
        else if(op.mask==0xffffffff)bus.write(op.address,4,op.value);
        else bus.write(op.address,4,(bb_init_read(bus,op.address)&~op.mask)|op.value);
    }
}
// Input must already have firmware headline/conditional selection applied by phy_table.py.
inline std::vector<BbInitOperation> selected_rf_nctl_plan(
        const std::vector<std::pair<unsigned,unsigned>>& selected_pairs) {
    if(selected_pairs.empty() || selected_pairs.size()>4096)
        throw std::invalid_argument("Invalid selected RF_NCTL table size");
    std::vector<BbInitOperation> plan;
    for(auto [address,value]:selected_pairs) {
        if(address>=0xf9 && address<=0xfe) {
            constexpr unsigned delays[]{1,5,50,1000,5000,50000};
            plan.push_back({0,0,0,delays[address-0xf9]});
        } else {
            if(address>0xfffc || address%4)
                throw std::invalid_argument("Unselected opcode or invalid RF_NCTL address");
            if(value!=0xbabecafe)plan.push_back({be_bb_register(address),0xffffffff,value,0});
        }
    }
    // A bypass-only selected table has no application effects.
    if(!plan.empty())validate_bb_init_plan(plan);
    return plan;
}
// Source performs up to 10000 write/delay/read attempts. Failure stops the caller;
// this readback handshake establishes NCTL register access, not MCU execution.
inline unsigned check_be_nctl(RegisterBus& bus,const std::function<void(unsigned)>& delay_us,
                              unsigned attempts=10000) {
    if(!attempts || attempts>10000 || !delay_us)
        throw std::invalid_argument("Invalid NCTL poll bound or missing delay callback");
    bus.write(0x28000,4,8);
    for(unsigned i=0;i<attempts;++i) {
        bus.write(0x28080,4,4); delay_us(10);
        if(bb_init_read(bus,0x28080)==4)return i+1;
    }
    throw std::runtime_error("NCTL initialization readback timeout");
}
} // namespace archer
