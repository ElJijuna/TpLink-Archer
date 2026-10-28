#include "archer/macos_bulk.hpp"
#include "archer/context_reservation.hpp"
#include "archer/passive_channels.hpp"
#include "archer/channel_bus.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/rfk_protocol.hpp"
#include "archer/rfk_log.hpp"
#include "archer/passive_scan.hpp"
#include "archer/rx_scan_window.hpp"
#include "archer/usb_rx_aggregation.hpp"
#include "archer/port_rx_window.hpp"
#include "archer/physts_window.hpp"
#include "archer/phy_report_window.hpp"
#include "channel_payload.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
static void delay_us(unsigned us){std::this_thread::sleep_for(std::chrono::microseconds(us));}
int main(int argc,char** argv){
    const bool sweep3=argc==3&&std::string(argv[1])=="--owned-slot0-channel1-6-11-1";
    if(argc!=3||(!sweep3&&std::string(argv[1])!="--owned-slot0-channel1-6-1")){
        std::cerr<<"Usage: archer-passive-retune {--owned-slot0-channel1-6-1|--owned-slot0-channel1-6-11-1} NEW_DIRECTORY\n";return 2;
    }
    try{
        archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
        const std::filesystem::path root=argv[2];
        if(!std::filesystem::create_directory(root))throw std::runtime_error("New capture directory required");
        // Validate all pure BB plans before I/O. No channel1 TX power table is
        // applied on channel6; this profile only issues PRE_NOTIFY and RXDCK.
        const archer::Channel1Inputs ready{true,true,true,true,true};
        const auto bb1=archer::passive_channel_bb_plan(1,ready,trusted_channel1_bb);
        const auto bb6=archer::passive_channel_bb_plan(6,ready,trusted_channel1_bb);
        const auto bb11=archer::passive_channel_bb_plan(11,ready,trusted_channel1_bb);
        MacRegisterBus bus;
        archer::with_wlan_io_window(bus,[&]{
            if((archer::channel_bus_read(bus,0xfc)&255)!=0x71||
               ((archer::channel_bus_read(bus,0xf0)>>12)&15)!=1||
               archer::read_chip_identity(bus).analog_id!=0x6933||
               (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9||
               (archer::channel_bus_read(bus,0x7880)&7)!=7||
               (archer::channel_bus_read(bus,0x10400)&0x1e04)||
               (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022||
               (archer::channel_bus_read(bus,0x84)&0x300007)!=7||
               (archer::channel_bus_read(bus,0x10398)&0x3ffff))throw std::runtime_error("Owned paused firmware profile missing");
            archer::BeRfTransport rf(bus);
            if((rf.read_full(0,0x18)&0x31fff)!=1||(rf.read_full(0,0x10018)&0x31fff)!=1||
               (rf.read_full(1,0x10018)&0x31fff)!=1)throw std::runtime_error("Initial channel1 readback absent");
            for(auto a:{0x28104u,0x28204u})if(archer::channel_bus_read(bus,a)&0x303)throw std::runtime_error("Owned slot0 selectors missing");
            ContextReservation reservation; // CLEAN admission; stays DIRTY on any ambiguity.
            std::vector<unsigned> sequences(sweep3?6:4);for(auto& s:sequences)s=ledger.reserve();
            unsigned seq_at=0,current_channel=1;
            std::array<unsigned,3> history{1,0,0}; // Explicit controlled-host-history option, not readback.
            std::unique_ptr<MacBulkChannel> usb;
            auto retain=[&](const std::filesystem::path& file,const std::vector<std::uint8_t>& bytes){
                std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());out.close();
                if(!out)throw std::runtime_error("Cannot retain retune evidence");
            };
            const auto transition=[&](unsigned target){
                const auto dir=root/("retune-"+std::to_string(target));std::filesystem::create_directory(dir);
                usb.reset(); // Abort/drain all reads before reset and discard queued host buffers.
                for(auto a:{0x11424u,0x11428u,0x1142cu})archer::channel_bus_write(bus,a,0xffff,0);
                archer::ChannelBus bracket(bus,delay_us);bracket.execute(archer::channel1_reset_enter({}));
                // BB packet detection is disabled. Drain old device-side USB
                // transfers before changing channel; no data is labeled as a dwell.
                {
                    MacBulkChannel drain;
                    bool quiet=false;
                    for(unsigned i=0;i<512;++i){bool any=false;
                        for(unsigned pipe=0;pipe<2;++pipe){auto bytes=drain.receive(pipe,10);if(bytes.empty())continue;any=true;retain(dir/("drain-"+std::to_string(i)+"-"+std::to_string(pipe)+".bin"),bytes);}
                        if(!any){quiet=true;break;}
                    }
                    if(!quiet)throw std::runtime_error("Previous channel did not quiesce");
                }
                for(const auto& op:target==1?bb1:target==6?bb6:bb11)archer::channel_bus_write(bus,op.address,op.mask,op.value);
                const std::array<unsigned,4> snap{rf.read_full(0,0x18),rf.read_full(0,0x10018),rf.read_full(1,0x18),rf.read_full(1,0x10018)};
                for(const auto& op:archer::passive_channel_rf_plan(target,ready,snap,1)){
                    rf.submit_full_write(op.path,op.address,op.full_value);delay_us(op.delay_after_us);
                    const auto actual=rf.read_full(op.path,op.address);
                    std::cout<<"{\"operation\":\"retune_rf_readback\",\"channel\":"<<target<<",\"path\":"<<op.path<<",\"address\":"<<op.address<<",\"value\":"<<actual<<"}"<<std::endl;
                    if((actual&0x31fff)!=target&&!(op.path==1&&op.address==0x18))throw std::runtime_error("Selected RF channel did not latch");
                }
                bracket.execute(archer::channel1_reset_exit({}));
                const unsigned slot=archer::passive_mcc_lookup(history,target,{target});
                const auto post=archer::passive_channel_post_rf_plan(target,{slot,slot},{rf.read_full(0,1),rf.read_full(1,1)});
                bracket.execute(post.operations);
                const auto rf_deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(5);
                archer::wait_rf_not_transmitting([&](unsigned p,unsigned a){return rf.read_full(p,a);},delay_us,
                    [&]{return std::chrono::steady_clock::now()>=rf_deadline;});
                auto next_history=history;next_history[slot]=target;
                usb=std::make_unique<MacBulkChannel>(true);
                archer::RfkPreNotifyV1 notify;notify.mlo_mode=1;notify.table_indices={slot,slot};
                for(unsigned p=0;p<2;++p)for(unsigned i=0;i<3;++i)notify.channels[p][i]=next_history[i];
                unsigned captures=0;
                const auto command=[&](archer::RfkCommand cmd,const std::vector<std::uint8_t>& payload){
                    // RFK terminal C2H has no sequence/command identity. Drain
                    // previous events before arming, under disabled RX filters.
                    auto quiet_since=std::chrono::steady_clock::now();
                    const auto drain_limit=quiet_since+std::chrono::seconds(3);bool quiet=false;
                    while(std::chrono::steady_clock::now()<drain_limit){
                        bool any=false;
                        for(unsigned pipe=0;pipe<2;++pipe){auto bytes=usb->receive(pipe,10);if(bytes.empty())continue;any=true;
                            if(captures>=1024)throw std::runtime_error("RFK drain capture limit");
                            retain(dir/("rx-"+std::to_string(captures++)+".bin"),bytes);
                        }
                        if(any)quiet_since=std::chrono::steady_clock::now();
                        if(std::chrono::steady_clock::now()-quiet_since>=std::chrono::milliseconds(50)){quiet=true;break;}
                    }
                    if(!quiet)throw std::runtime_error("RFK input did not quiesce before command");
                    archer::RfkReportTracker tracker;std::optional<archer::RxdckReport> report;
                    const auto frame=archer::wrap_h2c_usb_be(archer::encode_rfk_h2c(cmd,sequences.at(seq_at++),payload));
                    retain(dir/("tx-"+std::to_string(unsigned(cmd))+".bin"),frame);usb->send(frame);
                    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
                    do{
                        for(unsigned pipe=0;pipe<2;++pipe){auto bytes=usb->receive(pipe,10);if(bytes.empty())continue;
                            if(captures>=1024)throw std::runtime_error("Retune RFK capture limit");
                            retain(dir/("rx-"+std::to_string(captures++)+".bin"),bytes);
                            for(const auto& packet:archer::decode_usb_rx_be(bytes))if(packet.c2h){
                                const auto* p=bytes.data()+packet.payload_offset;tracker.observe(p,packet.payload_bytes);
                                for(const auto& r:archer::decode_rxdck_log_reports(p,packet.payload_bytes))report=r;
                            }
                        }
                    }while((!tracker.complete()||(tracker.success()&&cmd==archer::RfkCommand::rxdck&&!report))&&std::chrono::steady_clock::now()<end);
                    if(!tracker.success()||(cmd==archer::RfkCommand::rxdck&&(!report||!report->clean_2g20(target))))throw std::runtime_error("Retune RFK terminal/detail uncertain; no retry or automatic restore");
                    std::cout<<"{\"operation\":\"retune_rfk_complete\",\"channel\":"<<target<<",\"command\":"<<unsigned(cmd)<<",\"rxdck_clean\":"<<(cmd==archer::RfkCommand::rxdck?"true":"null")<<"}"<<std::endl;
                };
                const archer::RfkFirmwareVersion version{0,35,41,0};
                command(archer::RfkCommand::pre_notify,archer::rfk_pre_notify_payload(version,notify));
                command(archer::RfkCommand::rxdck,archer::rfk_rxdck_payload(version,0,{0,0,target},false,true));
                history=next_history;current_channel=target;
                if(archer::channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("Network TX changed");
            };
            const auto dwell=[&](unsigned channel){
                if(current_channel!=channel)throw std::runtime_error("Dwell channel is not committed");
                const auto dir=root/("channel-"+std::to_string(channel));std::filesystem::create_directory(dir);
                archer::PassiveScan scan(channel);unsigned transfers=0;
                archer::with_passive_rx_window(bus,[&]{
                    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
                    while(std::chrono::steady_clock::now()<end)for(unsigned pipe=0;pipe<2;++pipe){
                        auto bytes=usb->receive(pipe,10);if(bytes.empty())continue;
                        if(transfers>=1024)throw std::runtime_error("Dwell capture limit");
                        retain(dir/("rx-"+std::to_string(transfers++)+".bin"),bytes);
                        for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended))scan.observe(bytes,m);
                    }
                },false,archer::RxAcceptance::all_management);
                usb.reset();
                std::ofstream out(dir/"networks.json");scan.write_json(out);out.close();if(!out)throw std::runtime_error("Cannot write dwell result");
                std::cout<<"{\"operation\":\"passive_dwell_complete\",\"channel\":"<<channel<<",\"network_entries\":"<<scan.networks().size()<<",\"accepted_management\":"<<scan.accepted()<<",\"bad_fcs\":"<<scan.bad_fcs()<<"}"<<std::endl;
            };
            // Journal filter/response controls around the whole cycle. Failure
            // leaves the context reservation DIRTY and TX paused; no blind retune back.
            archer::with_usb_rx_aggregation(bus,[&]{archer::with_unlinked_port0_window(bus,[&]{archer::with_physts_window(bus,[&]{archer::with_phy_report_window(bus,[&]{
                archer::with_passive_rx_window(bus,[&]{transition(6);dwell(6);if(sweep3){transition(11);dwell(11);}transition(1);dwell(1);},false,archer::RxAcceptance::all_management);
            });});});});
            if(current_channel!=1||seq_at!=sequences.size()||(rf.read_full(0,0x10018)&0x31fff)!=1||(rf.read_full(1,0x10018)&0x31fff)!=1||
               (archer::channel_bus_read(bus,0x26b4c)&0x1fff)!=2412||(archer::channel_bus_read(bus,0x10398)&0x3ffff))throw std::runtime_error("Return to channel1 not verified");
            reservation.clean();
            std::cout<<"{\"result\":\""<<(sweep3?"passive_channel1_6_11_1_complete":"passive_channel1_6_1_complete")<<"\",\"network_tx_paused\":true,\"rxdck_all_channels_clean\":true,\"full_rf_quality_validated\":false}"<<std::endl;
        });return 0;
    }catch(const std::exception& e){std::cerr<<"passive-retune: "<<e.what()<<"; reservation may remain DIRTY, no replay\n";return 2;}
}
