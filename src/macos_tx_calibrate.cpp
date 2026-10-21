#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/cmac_init.hpp"
#include "archer/channel_bus.hpp"
#include "archer/trx_path.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/rf_power.hpp"
#include "channel_payload.hpp"
#include "tx_calibration_payload.hpp"
#include <filesystem>
#include <fstream>
static void delay_us(unsigned us){std::this_thread::sleep_for(std::chrono::microseconds(us));}
// Fixed research profile only: source-fresh host state, BTC intentionally not
// initialized, WW power tables, PHY0/SCC channel1/20MHz, scheduler never resumed.
int main(int argc,char** argv){
 if(argc!=3 || std::string(argv[1])!="--channel1-world-calibrate") {
    std::cerr<<"Usage: archer-tx-calibrate --channel1-world-calibrate NEW_CAPTURE_DIRECTORY\n";return 2;
 }
 try {
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
    const archer::Channel1Context context;
    const archer::RfkChannel channel{0,0,1};
    const archer::RfkFirmwareVersion version{0,35,41,0};
    // Validate all compile-bound board/planning data before touching hardware.
    const auto bb=archer::channel1_bb_plan(context,{true,true,true,true,true},trusted_channel1_bb);
    const auto mac=archer::channel1_mac_plan(context);
    const auto trim=archer::channel1_board_trim_plan(context,trusted_channel1_trim,0x6933,1,1);
    archer::validate_world_diagnostic_power(context,{0,0,0,true,true});
    const auto tssi_trim=archer::rtl8922a_linux_host_default_tssi_trim();
    (void)archer::prepare_tssi_normal_payload(version,0,channel,1,0,trusted_tssi_board,tssi_trim,trusted_tssi_tracking);
    if(!std::filesystem::create_directory(argv[2]))throw std::runtime_error("Capture directory must be new");
    const std::filesystem::path directory(argv[2]);
    MacRegisterBus bus;MacBulkChannel usb;
    archer::with_wlan_io_window(bus,[&]{
        if(archer::read_chip_identity(bus).analog_id!=0x6933 ||
           (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022 ||
           (archer::channel_bus_read(bus,0x84)&0x300007)!=7 ||
           (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9)
            throw std::runtime_error("Bound firmware/ADDA/BB profile absent");
        (void)archer::pause_cmac0_scheduler(bus);
        archer::initialize_usb2_scc_cmac0(bus,{true},[]{delay_us(1);});
        for(const auto& op:archer::rtl8922a_scc_bb_sethw_plan())
            archer::channel_bus_write(bus,op.address,op.mask,op.value);
        archer::initialize_rfk_pll(bus);
        std::cout<<"{\"phase\":\"cmac_completed\"}"<<std::endl;
        archer::BeRfTransport rf(bus);
        for(const auto& op:trim) {
            const auto old=rf.read_full(op.path,op.address);
            const auto value=archer::merge_channel_rf_field(old,op.mask,op.value);
            rf.submit_full_write(op.path,op.address,value);
            if((rf.read_full(op.path,op.address)&op.mask)!=(value&op.mask))throw std::runtime_error("Board RF trim did not latch");
        }
        archer::ChannelBus lifecycle(bus,delay_us);
        lifecycle.execute(archer::channel1_reset_enter(context));
        for(const auto& op:archer::rtl8922a_scc_ab_trx_path_plan({true}))
            archer::channel_bus_write(bus,op.address,op.mask,op.value);
        for(const auto& op:mac)archer::channel_bus_write(bus,op.address,op.mask,op.value,op.width);
        for(const auto& op:bb)archer::channel_bus_write(bus,op.address,op.mask,op.value);
        const std::array<unsigned,4> snapshot{rf.read_full(0,0x18),rf.read_full(0,0x10018),rf.read_full(1,0x18),rf.read_full(1,0x10018)};
        (void)rf.read_full(0,0x18); // Source's separate synthesizer snapshot.
        for(const auto& op:archer::channel1_rf_tuning_plan(context,{true,true,true,true,true},snapshot,1)) {
            rf.submit_full_write(op.path,op.address,op.full_value);delay_us(op.delay_after_us);
            const auto effective=rf.read_full(op.path,op.address);
            std::cout<<"{\"rf_channel_readback\":{\"path\":"<<op.path<<",\"address\":"<<op.address<<",\"effective\":"<<effective<<"}}"<<std::endl;
            // get_syn_sel(PHY0,MLO_DBCC_NOT_SUPPORT) selects A. Source also writes
            // B analog RF18 without enforcing equality; keep that observation
            // separate from selected SYN A and both digital channel registers.
            if((effective&0x31fff)!=1 && !(op.path==1 && op.address==0x18))
                throw std::runtime_error("Selected synthesizer/digital channel1 tuning did not latch");
        }
        for(const auto& op:trusted_channel1_world_power)
            archer::channel_bus_write(bus,op.address,op.mask,op.value);
        lifecycle.execute(archer::channel1_reset_exit(context));
        const auto post=archer::channel1_post_rf_plan(context,{0,0},{rf.read_full(0,1),rf.read_full(1,1)});
        lifecycle.execute(post.operations);
        std::cout<<"{\"phase\":\"channel1_world_applied\",\"network_tx_paused\":true}"<<std::endl;
        const auto rf_deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(5);
        archer::wait_rf_not_transmitting([&](unsigned p,unsigned a){return rf.read_full(p,a);},delay_us,
            [&]{return std::chrono::steady_clock::now()>=rf_deadline;});
        const archer::TxRfkPrerequisites prerequisites{true,true,true,true,true};
        // Fresh uninitialized BTC host profile: source !init_ok guard suppresses
        // hardware policy updates. Full production coexistence is not asserted.
        std::optional<archer::TxgapkReport> txgapk;
        std::optional<archer::IqkReport> iqk;
        std::optional<archer::DpkReport> dpk;
        std::optional<archer::TssiReport> tssi;
        std::optional<archer::RxdckReport> rxdck;
        unsigned capture=0;
        auto receive=[&](unsigned pipe,archer::RfkReportTracker* tracker){
            const auto bytes=usb.receive(pipe,5);
            if(bytes.empty())return false;
            const auto id=capture++;
            std::ofstream output(directory/("rx-"+std::to_string(id)+".bin"),std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            if(!output)throw std::runtime_error("Cannot retain RX capture");
            std::cout<<"{\"rx_capture\":"<<id<<",\"input_index\":"<<pipe<<",\"bytes\":"<<bytes.size()<<"}"<<std::endl;
            for(const auto& packet:archer::decode_usb_rx_be(bytes))if(packet.c2h && tracker){
                const auto* p=bytes.data()+packet.payload_offset;const auto n=packet.payload_bytes;
                for(const auto& r:archer::decode_txgapk_log_reports(p,n)) {
                    txgapk=r;std::cout<<"{\"txgapk_ok\":"<<r.ok<<",\"failcodes_present\":"<<(r.failcode?"true":"false")<<"}"<<std::endl;
                }
                for(const auto& r:archer::decode_iqk_log_reports(p,n)) {
                    iqk=r;std::cout<<"{\"iqk_flags_clear\":"<<(r.flags_clear()?"true":"false")<<",\"channel_matches\":"<<(r.matches(channel,0)?"true":"false")<<"}"<<std::endl;
                }
                for(const auto& r:archer::decode_dpk_log_reports(p,n)) {
                    dpk=r;std::cout<<"{\"dpk_error_flags_clear\":"<<(r.error_flags_clear()?"true":"false")<<",\"channel_matches\":"<<(r.matches(channel)?"true":"false")
                        <<",\"path_status\":["<<r.path_ok[0]<<","<<r.path_ok[1]<<"],\"path_success_known\":"<<(r.path_success().has_value()?"true":"false")<<"}"<<std::endl;
                }
                if(const auto log=archer::decode_rfk_log(p,n);log && log->function==1)
                    for(const auto& chunk:log->chunks)if(chunk.type==0 && chunk.content.size()==20)
                        std::cout<<"{\"dpk_run_format\":"<<archer::channel_u32(chunk.content.data())<<",\"args\":["<<archer::channel_u32(chunk.content.data()+4)<<","<<archer::channel_u32(chunk.content.data()+8)<<","<<archer::channel_u32(chunk.content.data()+12)<<","<<archer::channel_u32(chunk.content.data()+16)<<"]}"<<std::endl;
                for(const auto& r:archer::decode_tssi_log_reports(p,n)) {
                    tssi=r;std::cout<<"{\"tssi_alignment_state\":["<<r.alignment_state[0]<<","<<r.alignment_state[1]<<","<<r.alignment_state[2]<<","<<r.alignment_state[3]<<"],\"state_semantics_validated\":false}"<<std::endl;
                }
                for(const auto& r:archer::decode_rxdck_log_reports(p,n))rxdck=r;
                if(tracker->observe(p,n))std::cout<<"{\"rfk_state\":"<<static_cast<unsigned>(tracker->last->state)<<"}"<<std::endl;
            }
            return true;
        };
        auto command=[&](archer::RfkCommand cmd,const std::vector<std::uint8_t>& payload,bool scan=false){
            bool quiet=false;const auto end_drain=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            for(unsigned i=0;i<512 && std::chrono::steady_clock::now()<end_drain;++i) {
                const auto a=receive(0,nullptr),b=receive(1,nullptr);if(!a&&!b){quiet=true;break;}
            }
            if(!quiet)throw std::runtime_error("RX not quiescent before TX RFK");
            txgapk.reset();iqk.reset();dpk.reset();tssi.reset();rxdck.reset();
            const auto sequence=ledger.reserve();
            const auto h2c=cmd==archer::RfkCommand::tssi ?
                (scan?archer::encode_prepared_tssi_scan_h2c:archer::encode_prepared_tssi_normal_h2c)(sequence,version,0,channel,1,0,trusted_tssi_board,tssi_trim,trusted_tssi_tracking,prerequisites):
                archer::encode_rfk_h2c(cmd,sequence,payload);
            const auto frame=archer::wrap_h2c_usb_be(h2c);
            std::ofstream output(directory/("tx-"+std::to_string(sequence)+".bin"),std::ios::binary);
            output.write(reinterpret_cast<const char*>(frame.data()),frame.size());output.close();
            if(!output)throw std::runtime_error("Cannot preserve TX RFK command");
            archer::RfkReportTracker tracker;
            std::cout<<"{\"rfk_command\":"<<static_cast<unsigned>(cmd)<<",\"sequence\":"<<sequence<<",\"tssi_scan\":"<<(scan?"true":"false")<<"}"<<std::endl;
            usb.send(frame);
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            auto report_present=[&]{return scan || cmd==archer::RfkCommand::pre_notify ||
                (cmd==archer::RfkCommand::txgapk && txgapk) || (cmd==archer::RfkCommand::iqk && iqk) ||
                (cmd==archer::RfkCommand::tssi && tssi) || (cmd==archer::RfkCommand::dpk && dpk) || (cmd==archer::RfkCommand::rxdck && rxdck);};
            do {receive(0,&tracker);receive(1,&tracker);}while((!tracker.complete() || (tracker.success() && !report_present())) && std::chrono::steady_clock::now()<deadline);
            if(!tracker.success() || !report_present())throw std::runtime_error("TX RFK terminal/report missing or failed; no next command");
            if((txgapk && !txgapk->flags_clear()) || (iqk && (!iqk->flags_clear() || !iqk->matches(channel,0))) ||
               (dpk && (!dpk->error_flags_clear() || dpk->path_success()==std::optional<bool>{false} || !dpk->matches(channel))) || (rxdck && !rxdck->clean_initial_channel1()))
                throw std::runtime_error("TX RFK detailed report failed; no next command");
        };
        archer::RfkPreNotifyV1 notify;notify.mlo_mode=1; // Explicit SCC/no-DBCC profile.
        notify.channels[0][0]=notify.channels[1][0]=1;
        command(archer::RfkCommand::pre_notify,archer::rfk_pre_notify_payload(version,notify));
        command(archer::RfkCommand::tssi,{},true);
        const auto after_scan_deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(5);
        archer::wait_rf_not_transmitting([&](unsigned p,unsigned a){return rf.read_full(p,a);},delay_us,
            [&]{return std::chrono::steady_clock::now()>=after_scan_deadline;});
        command(archer::RfkCommand::pre_notify,archer::rfk_pre_notify_payload(version,notify));
        command(archer::RfkCommand::txgapk,archer::rfk_txgapk_payload(version,0,channel,1));
        command(archer::RfkCommand::iqk,archer::rfk_iqk_payload(version,0,channel,1,3));
        command(archer::RfkCommand::tssi,{});
        command(archer::RfkCommand::dpk,archer::rfk_dpk_payload(version,0,channel,true));
        command(archer::RfkCommand::rxdck,archer::rfk_rxdck_payload(version,0,channel,false,true));
        if(archer::channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("Scheduler no longer paused");
        std::cout<<"{\"result\":\"tx_rfk_commands_completed\",\"network_tx_paused\":true,\"all_calibration_quality_validated\":false,\"dpk_path_semantics_validated\":false,\"tssi_alignment_semantics_validated\":false}"<<std::endl;
    });
    return 0;
 }catch(const std::exception& e){std::cerr<<"tx-calibrate: "<<e.what()<<"; scheduler remains paused, do not retry pending RFK\n";return 2;}
}
