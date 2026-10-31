#ifdef ARCHER_SINGLE_PROBE
#include "archer/single_probe.hpp"
#include "archer/xtal_identity.hpp"
#include "channel_payload.hpp"
#include "tx_calibration_payload.hpp"
#endif
#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/interface_lifecycle.hpp"
#include "archer/port_rx_window.hpp"
#include "archer/rx_scan_window.hpp"
#include "archer/phy_report_window.hpp"
#include "archer/physts_window.hpp"
#include "archer/beacon.hpp"
#include "archer/passive_scan.hpp"
#include "archer/usb_rx_aggregation.hpp"
#include "archer/ple_probe.hpp"
#include "archer/rx_counters.hpp"
#include "archer/rx_delivery_audit.hpp"
#include <filesystem>
#include <fstream>
#include "archer/context_reservation.hpp"
int main(int argc,char** argv){
#ifdef ARCHER_SPECIAL_UNICAST
 constexpr unsigned probe_channel=9,probe_tag=3;
 const char* marker_path="artifacts/37ad-0104-special-channel9-unicast-probe-attempted";
#elif defined(ARCHER_SPECIAL_PROBE)
 constexpr unsigned probe_channel=9,probe_tag=2;
 const char* marker_path="artifacts/37ad-0104-special-channel9-probe-attempted";
#else
 constexpr unsigned probe_channel=1;
#ifdef ARCHER_SINGLE_PROBE
 constexpr unsigned probe_tag=1;
 const char* marker_path="artifacts/37ad-0104-single-probe-attempted";
#endif
#endif
#ifdef ARCHER_SINGLE_PROBE
#ifdef ARCHER_SPECIAL_UNICAST
 if(argc!=3||std::string(argv[1])!="--single-special-channel9-unicast-probe"){
#elif defined(ARCHER_SPECIAL_PROBE)
 if(argc!=3||std::string(argv[1])!="--single-special-channel9-probe"){
#else
 if(argc!=3||std::string(argv[1])!="--single-channel1-probe"){
#endif
    std::cerr<<"Usage: archer-single-probe --single-channel1-probe NEW_DIRECTORY\n";return 2;
 }
 argv[1]=const_cast<char*>("--run-owned-source-profile-usb-aggregation-all-management-async");
#endif
 if(argc!=3||(std::string(argv[1])!="--run-owned-source-profile"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation-all-management"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation-all-management-async")){
    std::cerr<<"Usage: archer-context-rx {--run-owned-source-profile|--run-owned-source-profile-usb-aggregation|--run-owned-source-profile-usb-aggregation-all-management|--run-owned-source-profile-usb-aggregation-all-management-async} NEW_DIRECTORY\n";return 2;
 }
 try{
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
#ifdef ARCHER_SINGLE_PROBE
    bool recovery_permit=false;
    if(const auto value=std::getenv("ARCHER_PROBE_RECOVERY_FD")){
        char* end=nullptr;errno=0;const long fd=std::strtol(value,&end,10);
        struct stat inherited{},target{};char state[19]{};
        if(errno||*end||fd<0||fd>0x7fffffff||fstat(fd,&inherited)||
           lstat(marker_path,&target)||
           !S_ISREG(target.st_mode)||target.st_nlink!=1||inherited.st_dev!=target.st_dev||inherited.st_ino!=target.st_ino||
           pread(fd,state,sizeof(state),0)!=18||std::string(state,18)!="RECOVERY_RESERVED\n"||flock(fd,LOCK_EX|LOCK_NB))
            throw std::runtime_error("Invalid inherited single-probe recovery reservation");
        recovery_permit=true;
    }
    if(!recovery_permit&&std::filesystem::exists(marker_path))
        throw std::runtime_error("A probe was already attempted; physical recovery and retry analysis required");
#endif
#ifdef ARCHER_SPECIAL_PROBE
    if(!recovery_permit)throw std::runtime_error("Directed probe requires coordinator reservation");
#endif
    const std::filesystem::path dir=argv[2];
    if(!std::filesystem::create_directory(dir))throw std::runtime_error("Capture directory must be new");
    MacRegisterBus bus;MacBulkChannel usb(std::string(argv[1])=="--run-owned-source-profile-usb-aggregation-all-management-async");
    archer::with_wlan_io_window(bus,[&]{
        if((archer::channel_bus_read(bus,0xfc)&255)!=0x71 ||
           (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022 ||
           (archer::channel_bus_read(bus,0x84)&0x300007)!=7 ||
           (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9 ||
           (archer::channel_bus_read(bus,0x10398)&0x3ffff) ||
           (archer::channel_bus_read(bus,0x10400)&0x1e04) ||
           !(archer::channel_bus_read(bus,0x9c00)&1))throw std::runtime_error("Owned SCC radio prerequisites absent");
        archer::BeRfTransport rf(bus);
        for(unsigned path=0;path<2;++path)if((rf.read_full(path,0x10018)&0x3ff)!=probe_channel)throw std::runtime_error("Channel1 absent");
        // Only this investigation has initialized firmware/CAM; no previous
        // context sender has run. Lock records ownership from this first run.
        ContextReservation reservation;
        bool submission_attempted=false;
        struct UnusedReservationGuard {
            ContextReservation& reservation;bool& attempted;
            ~UnusedReservationGuard(){if(!attempted)try{reservation.clean();}catch(...){} }
        } unused_guard{reservation,submission_attempted};
        std::array<unsigned,9> sequences{};for(auto& seq:sequences)seq=ledger.reserve();
        // Diagnostic locally administered address, never a discovered BSSID.
        const archer::UnlinkedCamContext context{{2,0,0,0x89,0x22,0xa1},0,0,0,0};
        // Cleanup target is unpaused/awake; initial hardware pause not measured.
        archer::InterfaceContextLifecycle session(context,sequences,true,true,false
#ifdef ARCHER_SINGLE_PROBE
            ,archer::StationCapability::legacy,0,true
#endif
        );
#ifdef ARCHER_SINGLE_PROBE
        bool probe_submitted=false;std::optional<archer::ProbeTxReport> probe_report;
#endif
        unsigned transfers=0,normal=0,beacons=0;
        archer::PassiveScan scan(probe_channel);
        const auto receive=[&](unsigned pipe,bool track){
            auto bytes=usb.receive(pipe,10);if(bytes.empty())return false;
            if(transfers>=1024)throw std::runtime_error("Context RX capture bound reached");
            std::ofstream file(dir/("rx-"+std::to_string(transfers++)+".bin"),std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.close();
            if(!file)throw std::runtime_error("Context RX capture write failed");
            for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
#ifdef ARCHER_SINGLE_PROBE
                if(m.packet.c2h&&probe_submitted){
                    auto rpt=archer::single_probe_report(bytes.data()+m.packet.payload_offset,m.packet.payload_bytes,probe_tag);
                    if(rpt){if(!probe_report&&archer::probe_report_terminal(*rpt,1))probe_report=rpt;
                        std::cout<<"{\"operation\":\"single_probe_tx_report\",\"tag\":"<<probe_tag<<",\"state\":"<<rpt->state<<",\"tx_count\":"<<rpt->count<<"}"<<std::endl;}
                }
#endif
                if(m.packet.c2h&&track){
                    const auto before=session.step();
                    if(session.observe(bytes.data()+m.packet.payload_offset,m.packet.payload_bytes))
                        std::cout<<"{\"operation\":\"context_ack\",\"step_before\":"<<before<<",\"step_after\":"<<session.step()<<"}"<<std::endl;
                }
                if(m.packet.packet_type==0){scan.observe(bytes,m);++normal;try{auto b=archer::decode_rx_beacon(bytes,m);if(!b.probe_response)++beacons;}catch(const std::invalid_argument&){} }
            }
            return true;
        };
        // Stop accepting new traffic only while draining pre-existing USB data.
        const std::array<unsigned,4> drain_regs{0x11424,0x11428,0x1142c,0x11440};
        const std::array<unsigned,4> drain_masks{0xffff,0xffff,0xffff,1};
        std::array<unsigned,4> drain_saved{};
        for(unsigned i=0;i<4;++i)drain_saved[i]=archer::channel_bus_read(bus,drain_regs[i]);
        std::exception_ptr drain_error;bool quiet=false;
        try{
            for(unsigned i=0;i<4;++i)archer::channel_bus_write(bus,drain_regs[i],drain_masks[i],0);
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            while(std::chrono::steady_clock::now()<deadline){const bool a=receive(0,false),b=receive(1,false);if(!a&&!b){quiet=true;break;}}
        }catch(...){drain_error=std::current_exception();}
        std::string drain_restore_errors;
        for(unsigned i=0;i<4;++i)try{archer::channel_bus_write(bus,drain_regs[i],drain_masks[i],drain_saved[i]&drain_masks[i]);}
            catch(const std::exception& e){drain_restore_errors+=e.what();}
        if(!drain_restore_errors.empty())throw std::runtime_error("Pre-probe RX restoration failed: "+drain_restore_errors);
        if(drain_error)std::rethrow_exception(drain_error);
        if(!quiet)throw std::runtime_error("Input did not quiesce; no context command sent");
        const auto measure=[&](const std::string& phase){
            archer::reset_rx_diagnostic_counters(bus);
            const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);
            while(std::chrono::steady_clock::now()<end){receive(0,true);receive(1,true);}
            constexpr std::array<unsigned,11> addresses{0x22e14,0x20e64,0x20e60,0x84,0x20704,0x20520,0x20c3c,0x20980,0x209a4,0x114f4,0x10000};
            std::array<unsigned,11> values{};
            for(unsigned i=0;i<addresses.size();++i)values[i]=archer::channel_bus_read(bus,addresses[i]);
            const auto a=rf.read_full(0,0),b=rf.read_full(1,0);
            std::cout<<"{\"operation\":\"context_phy_phase\",\"phase\":\""<<phase<<"\",\"values\":[";
            for(unsigned i=0;i<values.size();++i){if(i)std::cout<<',';std::cout<<values[i];}
            std::cout<<"],\"rf_mode_a\":"<<((a>>16)&15)<<",\"rf_mode_b\":"<<((b>>16)&15)<<"}"<<std::endl;
            constexpr std::array<unsigned,16> status_addresses{0x10164,0x110fc,0x110f8,0x114f4,0x114f8,0x110c0,0x110f4,0x11400,0x11020,0x9c00,0xb000,0xb010,0x8804,0x8808,0x880c,0x10000};
            std::array<unsigned,16> status{};
            for(unsigned i=0;i<status.size();++i)status[i]=archer::channel_bus_read(bus,status_addresses[i]);
            std::cout<<"{\"operation\":\"phy_mac_status\",\"phase\":\""<<phase<<"\",\"values\":[";
            for(unsigned i=0;i<status.size();++i){if(i)std::cout<<',';std::cout<<status[i];}
            std::cout<<"]}"<<std::endl;
        };
        const auto audit=[&](const std::string& phase){
            const auto values=archer::read_rx_delivery_audit(bus);
            for(unsigned i=0;i<values.size();++i){
                const auto& field=archer::rx_delivery_fields[i];
                std::cout<<"{\"operation\":\"rx_delivery_audit\",\"phase\":\""<<phase
                         <<"\",\"field\":\""<<field.name<<"\",\"address\":"<<field.address
                         <<",\"width\":"<<field.width<<",\"mask\":"<<field.mask
                         <<",\"source_value\":"<<field.source_value<<",\"value\":"<<values[i]
                         <<",\"source_match\":"<<((values[i]&field.mask)==field.source_value?"true":"false")<<"}"<<std::endl;
            }
            const auto phy_values=archer::read_phy_rx_delivery_audit(bus);
            for(unsigned i=0;i<phy_values.size();++i){
                const auto& field=archer::phy_rx_delivery_fields[i];
                std::cout<<"{\"operation\":\"phy_rx_delivery_audit\",\"phase\":\""<<phase
                         <<"\",\"field\":\""<<field.name<<"\",\"address\":"<<field.address
                         <<",\"width\":"<<field.width<<",\"mask\":"<<field.mask
                         <<",\"source_value\":"<<field.source_value<<",\"value\":"<<phy_values[i]
                         <<",\"source_match\":"<<((phy_values[i]&field.mask)==field.source_value?"true":"false")<<"}"<<std::endl;
            }
        };
        audit("before_windows");
        measure("before_windows");
        const auto command=[&]{
            const auto step=session.step();const auto policy=session.ack_policy();
            const auto frame=archer::wrap_h2c_usb_be(session.issue());
            std::ofstream file(dir/("tx-"+std::to_string(step)+".bin"),std::ios::binary);
            file.write(reinterpret_cast<const char*>(frame.data()),frame.size());file.close();
            if(!file)throw std::runtime_error("Context TX capture write failed");
            std::cout<<"{\"operation\":\"context_send\",\"step\":"<<step<<",\"sequence\":"<<sequences[step]<<",\"ack_policy\":"<<unsigned(policy)<<"}"<<std::endl;
            try{
                submission_attempted=true;usb.send(frame);session.sent_successfully();
                const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
                while(session.step()==step&&session.phase()!=archer::ContextPhase::uncertain&&std::chrono::steady_clock::now()<deadline){receive(0,true);receive(1,true);}
                if(session.step()==step&&session.phase()!=archer::ContextPhase::uncertain)session.mark_transport_uncertain();
                if(session.phase()==archer::ContextPhase::uncertain)throw std::runtime_error("Context command uncertain; no retry or automatic removal");
                measure("after_command_"+std::to_string(step));
            }catch(...){if(session.step()==step&&session.phase()!=archer::ContextPhase::uncertain)session.mark_transport_uncertain();throw;}
        };
        std::exception_ptr completed_tx_failure;
        const auto run_context=[&]{measure("after_aggregation");archer::with_unlinked_port0_window(bus,[&]{measure("after_port");archer::with_physts_window(bus,[&]{measure("after_physts");archer::with_phy_report_window(bus,[&]{measure("after_phy_report");
            archer::with_passive_rx_window(bus,[&]{
                measure("after_filters");
                for(unsigned i=0;i<6;++i)command();
                std::cout<<"{\"phase\":\"context_sequence_ready\",\"dmac_submission_only\":true}"<<std::endl;
                std::exception_ptr capture_error;
                try{
#ifdef ARCHER_SINGLE_PROBE
                    // Verify the world power fields actually remain resident.
                    for(const auto& op:probe_channel==9?trusted_channel9_world_power:trusted_channel1_world_power)
                        if((archer::channel_bus_read(bus,op.address)&op.mask)!=(op.value&op.mask))
                            throw std::runtime_error("World power readback absent; no probe sent");
                    const auto chip=archer::read_chip_identity(bus);
                    if(chip.revision!=1||chip.analog_id!=0x6933)throw std::runtime_error("Probe hardware profile mismatch");
                    if((archer::channel_bus_read(bus,0x26b4c)&0x1fff)!=(2407+5*probe_channel))
                        throw std::runtime_error("Probe BB channel1 absent");
                    if(!recovery_permit){
                    const auto marker=open(marker_path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
                    if(marker<0)throw std::runtime_error("Probe attempt already recorded; no automatic retransmission");
                    const char token[]="TAG1 RESERVED\n";
                    const bool persisted=write(marker,token,sizeof(token)-1)==sizeof(token)-1&&fsync(marker)==0;
                    close(marker);if(!persisted)throw std::runtime_error("Cannot persist single probe reservation");
                    }
                    const auto packet=probe_tag==3?archer::special_channel9_unicast_probe_packet():probe_channel==9?archer::special_channel9_probe_packet():archer::single_probe_packet();
                    std::ofstream probe_file(dir/"single-probe.bin",std::ios::binary);
                    probe_file.write(reinterpret_cast<const char*>(packet.data()),packet.size());probe_file.close();
                    if(!probe_file)throw std::runtime_error("Cannot preserve probe bytes");
                    try{
                        archer::channel_bus_write(bus,0x10398,0x3ffff,0x100); // MGQ only.
                        probe_submitted=true;usb.send_single_management_probe(packet);
                        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
                        while(!probe_report&&std::chrono::steady_clock::now()<deadline){receive(0,true);receive(1,true);}
                    }catch(...){archer::channel_bus_write(bus,0x10398,0x3ffff,0);probe_submitted=false;throw;}
                    archer::channel_bus_write(bus,0x10398,0x3ffff,0);
                    probe_submitted=false;
                    if(!probe_report)throw std::runtime_error("Probe report timeout; no retransmission");
                    if(probe_report->state!=0||probe_report->count==0)
                        throw std::runtime_error("Probe did not report a completed transmission");
#endif
                    audit("context_ready");
                    archer::reset_rx_diagnostic_counters(bus);
                    const auto start=std::chrono::steady_clock::now(),end=start+std::chrono::seconds(5);
                    const auto snapshot=[&]{
                        constexpr std::array<unsigned,10> addresses{0x114f4,0x10c00,0x10c84,0x8914,0x20e24,0x22e14,0x20e64,0x20e60,0x7880,0x10000};
                        std::array<unsigned,10> values{};
                        for(unsigned i=0;i<addresses.size();++i)values[i]=archer::channel_bus_read(bus,addresses[i]);
                        const auto empty=archer::ple_debug_read(bus,7,0),quota=archer::ple_debug_read(bus,1,6);
                        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
                        std::cout<<"{\"operation\":\"rx_path_snapshot\",\"elapsed_ms\":"<<ms<<",\"values\":[";
                        for(unsigned i=0;i<values.size();++i){if(i)std::cout<<',';std::cout<<values[i];}
                        std::cout<<"],\"ple_group0_empty\":"<<empty<<",\"quota6_raw\":"<<quota<<"}"<<std::endl;
                    };
                    snapshot();auto next=start+std::chrono::milliseconds(250);
                    while(std::chrono::steady_clock::now()<end){
                        receive(0,true);receive(1,true);
                        if(session.phase()==archer::ContextPhase::uncertain)throw std::runtime_error("Late context error");
                        if(std::chrono::steady_clock::now()>=next){snapshot();next=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);}
                    }
                    snapshot();
                }
                catch(...){capture_error=std::current_exception();}
                if(session.phase()==archer::ContextPhase::ready){session.begin_removal();for(unsigned i=0;i<3;++i)command();}
                if(capture_error){
#ifdef ARCHER_SINGLE_PROBE
                    // A terminal TX failure is distinct from a missing report.
                    // Finish verifying/restoring the acknowledged context before
                    // reporting that failure. Unknown transport/report stays DIRTY.
                    if(probe_report&&archer::probe_report_terminal(*probe_report,1)&&
                       session.phase()==archer::ContextPhase::cleanup_acknowledged)
                        completed_tx_failure=capture_error;
                    else
#endif
                    std::rethrow_exception(capture_error);
                }
            },false,(std::string(argv[1])=="--run-owned-source-profile-usb-aggregation-all-management"||std::string(argv[1])=="--run-owned-source-profile-usb-aggregation-all-management-async")?archer::RxAcceptance::all_management:archer::RxAcceptance::broadcast_bit_clear);
        });});});};
        if(std::string(argv[1])!="--run-owned-source-profile")archer::with_usb_rx_aggregation(bus,run_context);
        else run_context();
        measure("after_windows");
        audit("after_windows");
        if(session.phase()!=archer::ContextPhase::cleanup_acknowledged || (archer::channel_bus_read(bus,0x10398)&0x3ffff))throw std::runtime_error("Context cleanup/TX pause not confirmed");
        usb.close(); // Confirm callback shutdown before CLEAN, including known TX failure.
        reservation.clean();
        std::ofstream scan_file(dir/"networks.json");scan.write_json(scan_file);scan_file.close();
        if(!scan_file)throw std::runtime_error("Passive scan result write failed after context cleanup");
        std::cout<<"{\"operation\":\"passive_scan_summary\",\"network_entries\":"<<scan.networks().size()<<",\"accepted_management\":"<<scan.accepted()<<",\"bad_fcs\":"<<scan.bad_fcs()<<",\"table_overflow\":"<<scan.overflow()<<"}"<<std::endl;
        std::cout<<"{\"result\":\"context_cleanup_acknowledged\",\"transfers\":"<<transfers<<",\"normal_packets\":"<<normal<<",\"beacons\":"<<beacons<<",\"network_tx_paused\":true,\"initial_macid_pause_measured\":false}"<<std::endl;
        if(completed_tx_failure)std::rethrow_exception(completed_tx_failure);
    });return 0;
 }catch(const std::exception& e){std::cerr<<"context-rx: "<<e.what()<<"; reservation may remain DIRTY; no replay\n";return 2;}
}
