#pragma once
#include "archer/register_mailbox.hpp"
#include <array>
namespace archer {
struct RxDeliveryField {
    const char* name;unsigned address,width,mask,source_value;
};
// Read-only comparison with the pinned RTL8922A USB2/SCC initialization.
// A mismatch is evidence, not permission to repair a live register. Filter and
// aggregation windows intentionally differ from initialization in some phases.
inline constexpr std::array<RxDeliveryField,24> rx_delivery_fields{{
    {"shared_cmac",0xe000,4,0x40000007,0x40000007},
    {"ple_ready",0x9100,4,3,3},
    {"ple_layout",0x9008,4,0x1fff7f03,(2059u<<16)|(4u<<8)|2},
    {"ple_qta0",0x9040,4,0xffffffff,463u|(463u<<16)},
    {"ple_qta6",0x9058,4,0xffffffff,57u|(1509u<<16)},
    {"append_fcs",0x9c00,4,1,1},
    // Source requests bit24 but observed silicon clears it (stage32).
    {"cut_amsdu",0x9c94,4,0xfeffffff,0x200e05f0},
    {"rx_header_conversion",0x9cc0,4,0xffffffff,8},
    {"dispatcher_forward",0x8938,4,0xff,0x55},
    {"phy_interface",0x10000,4,0x20,0x20},
    {"address_cam",0x11434,4,0xff0001,0x7f0001},
    {"plcp_header_filter",0x11404,2,0x7f,0x6f},
    {"rmac_deadlock",0x11402,2,0xfff2,0x20f2},
    {"plcp_length_source",0x11514,2,0x100,0x100},
    {"report_queue_ids",0x10c0c,4,0x7efc0000,0x14280000},
    {"haxi_usb_receive",0xb000,4,0x7a0,0x420},
    {"hci_dma_enable",0x7880,4,7,7},
    {"usb_aggregation0",0x6000,4,0xffffffff,0x80ff2014},
    {"usb_aggregation1",0x6004,4,0xffffffff,0x1f},
    {"rx_filter_options",0x11420,4,0xffffffff,0x0f1740be},
    {"management_filter",0x11428,4,0xffffffff,0xffff},
    {"control_filter",0x11424,4,0xffffffff,0xffff},
    {"data_filter",0x1142c,4,0xffffffff,0xffff},
    {"busy_check_sequence",0x11400,2,0x8000,0x8000},
}};
inline std::array<unsigned,rx_delivery_fields.size()> read_rx_delivery_audit(RegisterBus& bus){
    std::array<unsigned,rx_delivery_fields.size()> values{};
    for(unsigned i=0;i<values.size();++i){
        const auto& field=rx_delivery_fields[i];
        const auto value=bus.read(field.address,field.width);
        if(value==0xdeadbeef||value==0xeaeaeaea)
            throw std::runtime_error("RX delivery audit register inaccessible");
        values[i]=value;
    }
    return values;
}
}
