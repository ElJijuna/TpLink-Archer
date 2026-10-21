#include "archer/tx_rfk.hpp"
#include <fstream>
#include <iterator>
#include <cassert>
#include <iostream>
int main(){
 using namespace archer;
 unsigned validated=0;
 for(const auto name:{"txgapk","iqk","tssi","dpk"}) {
    std::ifstream file(std::string("tests/fixtures/")+name+"-stage42-usb.bin",std::ios::binary);
    assert(file.good());const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    for(const auto& packet:decode_usb_rx_be(bytes)) {
        assert(packet.c2h);const auto* p=bytes.data()+packet.payload_offset;const auto n=packet.payload_bytes;
        for(const auto& r:decode_txgapk_log_reports(p,n)){assert(r.flags_clear() && !r.failcode);++validated;}
        for(const auto& r:decode_iqk_log_reports(p,n)){assert(r.flags_clear() && r.matches({0,0,1},0));++validated;}
        for(const auto& r:decode_tssi_log_reports(p,n)){assert((r.alignment_state==std::array<unsigned,4>{0,0,0,0}));++validated;}
        for(const auto& r:decode_dpk_log_reports(p,n)){
            assert(r.error_flags_clear() && r.matches({0,0,1}));
            assert((r.path_ok==std::array<unsigned,2>{4,4}) && !r.path_success().has_value());++validated;
        }
    }
 }
 assert(validated==4);
 std::cout<<"Physical TX RFK stage42 reports reproduced with unknown DPK/TSSI semantics preserved\n";
}
