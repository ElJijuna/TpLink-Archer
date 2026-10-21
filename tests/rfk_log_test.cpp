#include "archer/rfk_log.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
template<class F> void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    using namespace archer;
    std::ifstream f("tests/fixtures/dack-isolated-stage30-usb.bin",std::ios::binary);assert(f.good());
    const std::vector<std::uint8_t> capture{std::istreambuf_iterator<char>(f),{}};
    const auto packets=decode_usb_rx_be(capture);
    assert(capture.size()==152 && packets.size()==1 && packets[0].payload_bytes==128);
    const auto log=decode_rfk_log(capture.data()+24,128);
    assert(log && log->function==2 && log->chunks.size()==1);
    assert(log->chunks[0].type==1 && log->chunks[0].content.size()==117);
    const auto reports=decode_dack_log_reports(capture.data()+24,128);
    assert(reports.size()==1);
    const auto& physical=reports[0];
    assert(physical.calibration_version==13 && physical.info_version==2);
    assert((physical.timeout_flags==std::array<unsigned,7>{0,0,0,3,0,0,1}));
    assert(physical.dack_fail==0 && physical.has_failure_flags() && !physical.flags_clear());
    assert(physical.wbdck[0]==17 && physical.wbdck[1]==1 && physical.rck==0 && !physical.adgaink_ex);
    std::ifstream newer("tests/fixtures/dack-adda-stage31-usb.bin",std::ios::binary);assert(newer.good());
    const std::vector<std::uint8_t> stage31{std::istreambuf_iterator<char>(newer),{}};
    const auto newer_packets=decode_usb_rx_be(stage31);
    assert(newer_packets.size()==1);
    const auto repeated=decode_dack_log_reports(stage31.data()+newer_packets[0].payload_offset,
                                              newer_packets[0].payload_bytes);
    assert(repeated.size()==1 && repeated[0].timeout_flags[3]==3 && repeated[0].timeout_flags[6]==0);
    assert(repeated[0].dack_fail==0 && repeated[0].has_failure_flags());
    assert(std::any_of(repeated[0].raw.begin()+2,repeated[0].raw.begin()+106,
                       [](unsigned value){return value!=0;}));
    // We cannot establish whether MSBK flags are sticky; never erase them based on nonzero coefficients.
    RfkLogChunk clean=log->chunks[0];
    for(unsigned i=106;i<=113;++i)clean.content[i]=0;
    assert(decode_dack_report_v2(clean).flags_clear());
    for(unsigned i=106;i<=113;++i){
        auto failure=clean;failure.content[i]=1;
        assert(decode_dack_report_v2(failure).has_failure_flags());
    }
    auto extended=clean;extended.content.push_back(9);
    assert(decode_dack_report_v2(extended).adgaink_ex==9);
    auto unsupported=clean;unsupported.content[1]=1;
    rejects([&]{decode_dack_report_v2(unsupported);});
    auto truncated=clean;truncated.content.pop_back();
    rejects([&]{decode_dack_report_v2(truncated);});
    auto run=clean;run.type=0;
    rejects([&]{decode_dack_report_v2(run);});
    const auto foreign=encode_h2c(2,9,0,0,{1,0,5,0,0,0});
    assert(!decode_rfk_log(foreign.data(),foreign.size()));
    assert(decode_dack_log_reports(foreign.data(),foreign.size()).empty());
    for(auto malformed:{std::vector<std::uint8_t>{1,1},std::vector<std::uint8_t>{1,2,0,0},
                       std::vector<std::uint8_t>{2,1,0,0},std::vector<std::uint8_t>{1,0,0}}){
        auto wire=encode_h2c(2,8,2,0,malformed);
        rejects([&]{decode_rfk_log(wire.data(),wire.size());});
    }
    // Complete two-chunk message: RUN_LOG retained, supported RPT_LOG assessed.
    std::vector<std::uint8_t> content{0,1,0,0xaa,1,117,0};
    content.insert(content.end(),clean.content.begin(),clean.content.end());
    auto combined=encode_h2c(2,8,2,0,content);
    assert(decode_rfk_log(combined.data(),combined.size())->chunks.size()==2);
    assert(decode_dack_log_reports(combined.data(),combined.size())[0].flags_clear());
    auto rxwire=encode_h2c(2,8,3,0,{1,9,0,4,0,0,0,0,1,1,0,0});
    auto rxreports=decode_rxdck_log_reports(rxwire.data(),rxwire.size());
    assert(rxreports.size()==1 && rxreports[0].clean_initial_channel1());
    rxwire.back()=1;
    assert(!decode_rxdck_log_reports(rxwire.data(),rxwire.size())[0].clean_initial_channel1());
    rxwire=encode_h2c(2,8,3,0,{1,8,0,4,0,0,0,0,1,1,0});
    rejects([&]{decode_rxdck_log_reports(rxwire.data(),rxwire.size());});
    for(const auto name:{"tests/fixtures/dack-clean-stage34-usb.bin","tests/fixtures/rxdck-clean-stage34-usb.bin"}) {
        std::ifstream input(name,std::ios::binary);assert(input.good());
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};
        const auto decoded=decode_usb_rx_be(bytes);assert(decoded.size()==1 && decoded[0].c2h);
        const auto* payload=bytes.data()+decoded[0].payload_offset;
        const auto dack=decode_dack_log_reports(payload,decoded[0].payload_bytes);
        const auto rxdck=decode_rxdck_log_reports(payload,decoded[0].payload_bytes);
        assert((dack.size()==1 && dack[0].flags_clear()) ||
               (rxdck.size()==1 && rxdck[0].clean_initial_channel1()));
    }
    std::cout<<"RFK log tests passed\n";
}
