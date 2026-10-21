#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/rfk_protocol.hpp"
#include "archer/rfk_log.hpp"
#include "archer/rf_power.hpp"
#include <filesystem>
#include <fstream>
int main(int argc,char** argv){
 if(argc!=3 || (std::string(argv[1])!="--initial-pre-dack" && std::string(argv[1])!="--initial-rfk" && std::string(argv[1])!="--adda-pre-dack" && std::string(argv[1])!="--drain-only")) {
    std::cerr<<"Usage: archer-rfk [--initial-pre-dack | --initial-rfk | --adda-pre-dack | --drain-only] NEW_CAPTURE_DIRECTORY\n";return 2;
 }
 try {
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
    if(!std::filesystem::create_directory(argv[2]))throw std::runtime_error("Capture directory must be new");
    const std::filesystem::path dir(argv[2]);
    MacRegisterBus bus;MacBulkChannel usb;unsigned capture=0;
    std::optional<archer::DackReportV2> dack_report;
    std::optional<archer::RxdckReport> rxdck_report;
    archer::with_wlan_io_window(bus,[&]{
        if((bus.read(0x8d00,4)&3)!=3 || (bus.read(0x9100,4)&3)!=3 ||
           (bus.read(0x84,4)&0x33)!=3 || (bus.read(0x842c,4)&0xffff)!=0x7ff9)
            throw std::runtime_error("DLE/BB prerequisites absent");
        auto receive=[&](unsigned pipe,archer::RfkReportTracker* tracker){
            auto bytes=usb.receive(pipe,5);
            if(bytes.empty())return false;
            std::ofstream out(dir/("rx-"+std::to_string(capture++)+".bin"),std::ios::binary);
            out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            if(!out)throw std::runtime_error("Cannot retain RX");
            for(const auto& p:archer::decode_usb_rx_be(bytes))if(p.c2h){
                for(const auto& report:archer::decode_dack_log_reports(bytes.data()+p.payload_offset,p.payload_bytes)) {
                    dack_report=report;
                    std::cout<<"{\"dack_quality\":{\"msbk_timeout\":"<<report.timeout_flags[3]
                             <<",\"drck_timeout\":"<<report.timeout_flags[6]<<",\"failure_flags\":"
                             <<(report.has_failure_flags()?"true":"false")<<"}}"<<std::endl;
                }
                if(!tracker)continue;
                for(const auto& report:archer::decode_rxdck_log_reports(bytes.data()+p.payload_offset,p.payload_bytes)) {
                    rxdck_report=report;
                    std::cout<<"{\"rxdck_quality\":{\"channel_a\":"<<report.channel[0]<<",\"channel_b\":"<<report.channel[1]
                             <<",\"timeout_a\":"<<report.timeout[0]<<",\"timeout_b\":"<<report.timeout[1]<<"}}"<<std::endl;
                }
                if(tracker->observe(bytes.data()+p.payload_offset,p.payload_bytes))
                    std::cout<<"{\"rfk_state\":"<<static_cast<unsigned>(tracker->last->state)
                             <<",\"version\":"<<tracker->last->version<<"}"<<std::endl;
            }
            return true;
        };
        auto command=[&](archer::RfkCommand cmd,const std::vector<std::uint8_t>& payload,unsigned ms){
            bool quiet=false;
            const auto drain_end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            for(unsigned round=0;round<512 && std::chrono::steady_clock::now()<drain_end;++round){
                const bool a=receive(0,nullptr),b=receive(1,nullptr);
                if(!a&&!b){quiet=true;break;}
            }
            if(!quiet)throw std::runtime_error("RFK input did not quiesce; no command sent");
            const auto seq=ledger.reserve();
            if(cmd==archer::RfkCommand::dack)dack_report.reset();
            if(cmd==archer::RfkCommand::rxdck)rxdck_report.reset();
            const auto frame=archer::wrap_h2c_usb_be(archer::encode_rfk_h2c(cmd,seq,payload));
            std::ofstream tx(dir/("tx-"+std::to_string(seq)+".bin"),std::ios::binary);
            tx.write(reinterpret_cast<const char*>(frame.data()),frame.size());tx.close();
            if(!tx)throw std::runtime_error("Cannot retain RFK command");
            archer::RfkReportTracker tracker;
            std::cout<<"{\"rfk_command\":"<<static_cast<unsigned>(cmd)<<",\"sequence\":"<<seq<<"}"<<std::endl;
            usb.send(frame);
            const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);
            auto waiting=[&]{
                if(!tracker.complete())return true;
                if(!tracker.success())return false;
                return (cmd==archer::RfkCommand::dack && !dack_report) ||
                       (cmd==archer::RfkCommand::rxdck && !rxdck_report);
            };
            do {receive(0,&tracker);receive(1,&tracker);}while(waiting() && std::chrono::steady_clock::now()<end);
            if(!tracker.success())throw std::runtime_error(tracker.complete()?"RFK firmware rejected command":"RFK terminal report absent; no retry or next command");
            if(cmd==archer::RfkCommand::dack && (!dack_report || dack_report->has_failure_flags()))
                throw std::runtime_error("DACK terminal OK but detailed report missing or failure flags present");
            if(cmd==archer::RfkCommand::rxdck && (!rxdck_report || !rxdck_report->clean_initial_channel1()))
                throw std::runtime_error("RXDCK terminal OK but detailed channel/timeout report not clean");
        };
        if(std::string(argv[1])=="--drain-only") {
            archer::RfkReportTracker tracker;
            const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);
            do {receive(0,&tracker);receive(1,&tracker);}while(std::chrono::steady_clock::now()<end);
            std::cout<<"{\"result\":\"passive_drain\",\"terminal_report\":"<<(tracker.complete()?"true":"false")<<"}"<<std::endl;
            return;
        }
        const archer::RfkFirmwareVersion version{0,35,41,0};
        if(std::string(argv[1])=="--adda-pre-dack")archer::enable_adda_power(bus);
        archer::RfkPreNotifyV1 initial;
        initial.mlo_mode=1; // Explicit SCC/no-DBCC profile; zeroed initial MCC slots.
        command(archer::RfkCommand::pre_notify,archer::rfk_pre_notify_payload(version,initial),20);
        command(archer::RfkCommand::dack,archer::rfk_dack_payload(version,0),2000);
        if(std::string(argv[1])=="--initial-rfk")
            command(archer::RfkCommand::rxdck,archer::rfk_rxdck_payload(version,0,{0,0,1}),2000);
        std::cout<<"{\"result\":\""<<(std::string(argv[1])=="--initial-rfk"?"initial_pre_dack_rxdck_clean":"pre_notify_dack_clean")<<"\",\"full_calibration_complete\":false}"<<std::endl;
    });
    return 0;
 }catch(const std::exception& e){std::cerr<<"rfk: "<<e.what()<<'\n';return 2;}
}
