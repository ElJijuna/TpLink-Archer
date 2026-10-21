#pragma once
#include "archer/register_mailbox.hpp"
#include <optional>
#include <vector>
namespace archer {
// RTL8922A power-on tail enables these before sys_init/trx_init. The generic
// sys_init shared-CMAC helper is a no-op on RTL8922A and does not repair it.
inline void enable_8922a_shared_cmac(RegisterBus& bus,bool exclusive_initialization_owner) {
    if(!exclusive_initialization_owner)
        throw std::runtime_error("Shared CMAC enable requires exclusive initialization ownership");
    auto read=[&](unsigned address){
        const auto value=bus.read(address,4);
        if(value==0xeaeaeaea || value==0xdeadbeef)
            throw std::runtime_error("Shared CMAC control inaccessible");
        return value;
    };
    if((read(0xfc)&255)!=0x71 || ((read(0xf0)>>12)&15)!=1 ||
       (read(0x8400)&0x60000000)!=0x60000000)
        throw std::runtime_error("Shared CMAC requires RTL8922A CV1 and MAC/DMAC power prerequisites");
    constexpr unsigned mask=0x40000007; // SHARE, RESPBA, ADDRSRCH, BTCOEX
    bus.write(0xe000,4,read(0xe000)|mask);
    if((read(0xe000)&mask)!=mask)
        throw std::runtime_error("Shared CMAC enable did not latch; reset required");
}
struct DmacReservedMpduInfo {
    unsigned ple_free_pages, ple_page_bytes, mpdu_reserved_pages, band1_offset;
};
struct DmacRemainderInputs {
    bool exclusive_initialization_owner=false;
    bool usb2_scc_dle_ready=false;
    bool usb2_scc_hfc_initialized=false;
    std::optional<DmacReservedMpduInfo> reserved_mpdu;
};
enum class DmacInitKind { masked_write, poll_set };
struct DmacInitOp {
    DmacInitKind kind;
    unsigned address, width, mask, value, timeout_us=0;
    bool verify=true;
};
inline std::vector<DmacInitOp> usb2_scc_dmac_remainder_plan(const DmacRemainderInputs& inputs) {
    if(!inputs.exclusive_initialization_owner || !inputs.usb2_scc_dle_ready ||
       !inputs.usb2_scc_hfc_initialized || !inputs.reserved_mpdu)
        throw std::runtime_error("DMAC remainder requires exclusive lifecycle, DLE, HFC and reserved MPDU metadata");
    const auto& mpdu=*inputs.reserved_mpdu;
    // This primitive is deliberately fixed to the pinned USB2 SCC table:
    // ple_size6_v1, ple_rsvd_qt2, no dle_input => MPDU_INFO_B1_OFST=18.
    if(mpdu.ple_free_pages!=2059 || mpdu.ple_page_bytes!=256 ||
       mpdu.mpdu_reserved_pages!=1 || mpdu.band1_offset!=18)
        throw std::runtime_error("Reserved MPDU metadata does not match RTL8922A USB2 SCC");
    using K=DmacInitKind;
    return {
        {K::masked_write,0xa310,1,1,1},
        {K::poll_set,0xa310,4,0x80000000,0x80000000,2000},
        {K::masked_write,0xa310,4,0x20000000,0x20000000,0,false},
        {K::masked_write,0xa310,4,0x10000200,0},
        {K::masked_write,0x9c00,4,1,1},
        {K::masked_write,0x9c94,4,0xffffffff,0x210e05f0},
        // Linux write32_set(old-with-bit4-cleared) retains original bit4.
        // Match the effective source operation: set bits2:0, preserve bit4.
        {K::masked_write,0x9b00,4,7,7},
        {K::masked_write,0x9cc0,4,0xffffffff,8},
        {K::masked_write,0x8938,4,0xff,0x55},
        {K::masked_write,0x9d00,4,0xf3f,0xf3f},
        {K::masked_write,0x9d04,4,3,3},
        {K::masked_write,0x9f10,4,0x8fff003f,0x80000000|(2059u<<16)|18},
        {K::masked_write,0xa114,4,0x800000,0x800000,0,false},
        {K::masked_write,0xa114,4,0x800000,0},
        {K::poll_set,0xa114,4,0x80000000,0x80000000,1000},
        {K::masked_write,0xa310,4,0x400,0x400},
        {K::masked_write,0xe010,4,1,1}
    };
}
inline unsigned initialize_usb2_scc_dmac_remainder(RegisterBus& bus,
    const DmacRemainderInputs& inputs,const std::function<void()>& delay_one_us) {
    const auto plan=usb2_scc_dmac_remainder_plan(inputs); // Validate before I/O.
    if(!delay_one_us)throw std::runtime_error("DMAC bounded poll requires >=1us delay");
    auto read=[&](unsigned address,unsigned width){
        const auto value=bus.read(address,width);
        if(value==0xeaeaeaea || value==0xdeadbeef)
            throw std::runtime_error("DMAC register inaccessible; reset required");
        return value;
    };
    if((read(0xfc,4)&255)!=0x71 || ((read(0xf0,4)>>12)&15)!=1 ||
       (read(0x8400,4)&0x60000000)!=0x60000000 ||
       (read(0xe000,4)&0x40000007)!=0x40000007 ||
       (read(0x8d00,4)&3)!=3 || (read(0x9100,4)&3)!=3)
        throw std::runtime_error("DMAC prerequisites/readiness failed before writes");
    // No generic rollback after scheduler/MPDU/MLO initialization. Stop at the
    // first failure; lifecycle owner must keep producers paused and reset.
    for(const auto& op:plan){
        if(op.kind==DmacInitKind::poll_set){
            bool ready=false;
            for(unsigned elapsed=0;elapsed<=op.timeout_us;++elapsed){
                if((read(op.address,op.width)&op.mask)==op.value){ready=true;break;}
                if(elapsed<op.timeout_us)delay_one_us();
            }
            if(!ready)throw std::runtime_error(op.address==0xa310 ?
                "STA scheduler initialization timeout; reset required" : "MLO table initialization timeout; reset required");
        } else {
            const auto value=op.mask==0xffffffff ? op.value :
                (read(op.address,op.width)&~op.mask)|op.value;
            bus.write(op.address,op.width,value);
            // CV1 captures stage32c/d clear CUT_CTRL bit24 after the source
            // request, even with shared CMAC enabled. Linux does not require
            // this bit to latch. Preserve the request and report effective state.
            const auto verification_mask=op.address==0x9c94 ? op.mask&~0x01000000u : op.mask;
            if(op.verify && (read(op.address,op.width)&verification_mask)!=(op.value&verification_mask))
                throw std::runtime_error("DMAC write did not latch; reset required");
        }
    }
    return read(0x9c94,4);
}
}
