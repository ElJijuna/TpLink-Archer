#include "archer/rfk_protocol.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}
int main() {
    using namespace archer;
    const RfkFirmwareVersion firmware{0,35,41,0};
    RfkPreNotifyV1 pre;
    pre.channels = {{{1,6,11},{36,42,58}}};
    pre.bands = {{{0,0,0},{1,1,1}}};
    pre.table_indices = {2,1}; pre.phy = 1; pre.mlo_mode = 0x12345678; pre.mlo_1_1 = true;
    const auto p = rfk_pre_notify_payload(firmware, pre);
    assert(p.size() == 76);
    const std::array<std::uint32_t,19> words{1,6,11,36,42,58,0,0,0,1,1,1,0x12345678,11,42,0,1,1,1};
    for (unsigned i=0; i<words.size(); ++i) assert(channel_u32(p.data()+4*i) == words[i]);
    const auto h = encode_rfk_h2c(RfkCommand::pre_notify, 17, p);
    assert(channel_u32(h.data()) == 0x1100082e && channel_u32(h.data()+4) == 84);
    assert((rfk_dack_payload(firmware, 0) == std::vector<std::uint8_t>{3,0,0}));
    const auto d = encode_rfk_h2c(RfkCommand::dack, 0, rfk_dack_payload(firmware, 1));
    assert(channel_u32(d.data()) == 0x52e && channel_u32(d.data()+4) == 11);
    assert((rfk_rxdck_payload(firmware, 1, {1,2,42}, true, true) ==
            std::vector<std::uint8_t>{8,1,0,3,1,2,42,1}));
    assert((rfk_rxdck_payload({0,35,42,0}, 0, {0,0,6}, false, true) ==
            std::vector<std::uint8_t>{9,0,0,3,0,0,6,0,1}));
    rejects([&]{ rfk_dack_payload({0,35,30,255}, 0); });
    rejects([&]{ rfk_dack_payload({0,35,49,0}, 0); });
    rejects([&]{ rfk_dack_payload(firmware, 2); });
    rejects([&]{ rfk_rxdck_payload(firmware, 0, {0,2,6}); });
    pre.table_indices[1] = 3;
    rejects([&]{ rfk_pre_notify_payload(firmware, pre); });
    assert((rfk_txgapk_payload(firmware,0,{1,2,42},1)==
            std::vector<std::uint8_t>{8,2,0,3,1,2,42,1}));
    assert((rfk_iqk_payload(firmware,1,{0,1,6},1,2)==
            std::vector<std::uint8_t>{8,0,1,2,0,1,6,1}));
    assert((rfk_dpk_payload(firmware,0,{2,4,31},true)==
            std::vector<std::uint8_t>{8,0,1,3,2,4,31,1}));
    for(auto command:{RfkCommand::txgapk,RfkCommand::iqk,RfkCommand::dpk}) {
        auto wire=encode_rfk_h2c(command,42,{8,0,0,3,0,0,6,1});
        assert(channel_u32(wire.data())==(0x2a00002e | (static_cast<unsigned>(command)<<8)));
        assert(channel_u32(wire.data()+4)==16);
    }
    rejects([&]{rfk_iqk_payload(firmware,0,{0,0,6},1,0);});
    rejects([&]{rfk_iqk_payload(firmware,0,{0,0,6},1,12);});
    rejects([&]{rfk_txgapk_payload(firmware,0,{0,0,6},256);});
    rejects([&]{rfk_dpk_payload({0,35,28,0},0,{0,0,6});});
    rejects([&]{rfk_iqk_payload({0,35,27,255},0,{0,0,6},1,3);});
    rejects([&]{encode_rfk_h2c(RfkCommand::tssi,0,std::vector<std::uint8_t>(300));});
    bool tssi_blocked=false;
    try {rfk_tssi_normal_payload(firmware,0,{0,0,6},1,1);}
    catch(const std::runtime_error&){tssi_blocked=true;}
    assert(tssi_blocked);
    // Synthetic C2H, not a captured physical completion.
    auto report = encode_h2c(2,9,0,0,{0,7});
    RfkReportTracker tracker;
    assert(tracker.observe(report.data(), report.size()) && !tracker.complete());
    const auto ack = encode_h2c(1,0,1,0,{0,0,0,0});
    assert(!tracker.observe(ack.data(), ack.size()) && !tracker.complete());
    report[8] = 1;
    assert(tracker.observe(report.data(), report.size()) && tracker.success());
    assert(tracker.observe(report.data(), report.size()));
    report[8] = 2;
    rejects([&]{ tracker.observe(report.data(), report.size()); });
    for (unsigned state=2; state<=4; ++state) {
        RfkReportTracker failed;
        report[8] = state;
        assert(failed.observe(report.data(), report.size()) && failed.complete() && !failed.success());
    }
    report[8] = 5;
    rejects([&]{ decode_rfk_report(report.data(), report.size()); });
    report[8] = 1;
    rejects([&]{ decode_rfk_report(report.data(), 9); });
    auto short_report = encode_h2c(2,9,0,0,{1});
    rejects([&]{ decode_rfk_report(short_report.data(), short_report.size()); });
    auto foreign = encode_h2c(2,9,6,0,{1,0});
    assert(!decode_rfk_report(foreign.data(), foreign.size()));
    std::ifstream capture("tests/fixtures/rfk-pre-notify-stage30-usb.bin", std::ios::binary);
    assert(capture.good());
    const std::vector<std::uint8_t> recorded{std::istreambuf_iterator<char>(capture), {}};
    const auto packets=decode_usb_rx_be(recorded);
    assert(recorded.size()==38 && packets.size()==1 && packets[0].packet_type==10);
    assert(packets[0].payload_bytes==14 && packets[0].payload_offset==24);
    const auto captured=decode_rfk_report(recorded.data()+24,14);
    assert(captured && captured->state==RfkReportState::ok && captured->version==0);
    assert((captured->opaque_extension==std::vector<std::uint8_t>{8,0,0,0}));
    RfkReportTracker actual;
    assert(actual.observe(recorded.data()+24,14) && actual.success());
    assert(actual.observe(recorded.data()+24,14));
    // Tail is opaque: even value8 must not be used to match a PRE_NOTIFY command.
    auto alternate=std::vector<std::uint8_t>(recorded.begin()+24,recorded.end());
    alternate[10]=5;
    const auto alternate_report=decode_rfk_report(alternate.data(),alternate.size());
    assert(alternate_report && alternate_report->opaque_extension[0]==5);
    rejects([&]{actual.observe(alternate.data(),alternate.size());});
    for(unsigned tail_size:{1u,2u,3u,5u}) {
        auto malformed=encode_h2c(2,9,0,0,std::vector<std::uint8_t>(2+tail_size,0));
        rejects([&]{decode_rfk_report(malformed.data(),malformed.size());});
    }
    std::cout << "RFK protocol tests passed\n";
}
