#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/interface_lifecycle.hpp"
#include "archer/port_rx_window.hpp"
#include "archer/rx_scan_window.hpp"
#include "archer/phy_report_window.hpp"
#include "archer/physts_window.hpp"
#include "archer/beacon.hpp"
#include "archer/usb_rx_aggregation.hpp"
#include "archer/ple_probe.hpp"
#include "archer/rx_counters.hpp"
#include <filesystem>
#include <fstream>
class ContextReservation {
 int fd=-1;
public:
 ContextReservation(){
    fd=open("artifacts/37ad-0104-context-reservation",O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(fd<0)throw std::runtime_error("Cannot open context reservation");
    struct stat st{};char state[6]{};
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||flock(fd,LOCK_EX|LOCK_NB)||
       (st.st_size && (st.st_size!=6||pread(fd,state,6,0)!=6||std::string(state,6)!="CLEAN\n"))){
        close(fd);fd=-1;throw std::runtime_error("Context reservation dirty/owned; recover firmware before reuse");
    }
    try{write_state("DIRTY\n");}catch(...){close(fd);fd=-1;throw;}
 }
 void write_state(const char* state){if(pwrite(fd,state,6,0)!=6||ftruncate(fd,6)||fsync(fd))throw std::runtime_error("Context reservation persistence failed");}
 void clean(){write_state("CLEAN\n");}
 ~ContextReservation(){if(fd>=0)close(fd);}
};
int main(int argc,char** argv){
 if(argc!=3||(std::string(argv[1])!="--run-owned-source-profile"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation-all-management"&&std::string(argv[1])!="--run-owned-source-profile-usb-aggregation-all-management-async")){
    std::cerr<<"Usage: archer-context-rx {--run-owned-source-profile|--run-owned-source-profile-usb-aggregation|--run-owned-source-profile-usb-aggregation-all-management|--run-owned-source-profile-usb-aggregation-all-management-async} NEW_DIRECTORY\n";return 2;
 }
 try{
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
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
        for(unsigned path=0;path<2;++path)if((rf.read_full(path,0x10018)&0x3ff)!=1)throw std::runtime_error("Channel1 absent");
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
        archer::InterfaceContextLifecycle session(context,sequences,true,true,false);
        unsigned transfers=0,normal=0,beacons=0;
        const auto receive=[&](unsigned pipe,bool track){
            auto bytes=usb.receive(pipe,10);if(bytes.empty())return false;
            if(transfers>=1024)throw std::runtime_error("Context RX capture bound reached");
            std::ofstream file(dir/("rx-"+std::to_string(transfers++)+".bin"),std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.close();
            if(!file)throw std::runtime_error("Context RX capture write failed");
            for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
                if(m.packet.c2h&&track){
                    const auto before=session.step();
                    if(session.observe(bytes.data()+m.packet.payload_offset,m.packet.payload_bytes))
                        std::cout<<"{\"operation\":\"context_ack\",\"step_before\":"<<before<<",\"step_after\":"<<session.step()<<"}"<<std::endl;
                }
                if(m.packet.packet_type==0){++normal;try{auto b=archer::decode_rx_beacon(bytes,m);if(!b.probe_response)++beacons;}catch(const std::invalid_argument&){} }
            }
            return true;
        };
        bool quiet=false;const auto drain=std::chrono::steady_clock::now()+std::chrono::seconds(3);
        while(std::chrono::steady_clock::now()<drain){const bool a=receive(0,false),b=receive(1,false);if(!a&&!b){quiet=true;break;}}
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
        const auto run_context=[&]{measure("after_aggregation");archer::with_unlinked_port0_window(bus,[&]{measure("after_port");archer::with_physts_window(bus,[&]{measure("after_physts");archer::with_phy_report_window(bus,[&]{measure("after_phy_report");
            archer::with_passive_rx_window(bus,[&]{
                measure("after_filters");
                for(unsigned i=0;i<6;++i)command();
                std::cout<<"{\"phase\":\"context_sequence_ready\",\"dmac_submission_only\":true}"<<std::endl;
                std::exception_ptr capture_error;
                try{
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
                if(capture_error)std::rethrow_exception(capture_error);
            },false,(std::string(argv[1])=="--run-owned-source-profile-usb-aggregation-all-management"||std::string(argv[1])=="--run-owned-source-profile-usb-aggregation-all-management-async")?archer::RxAcceptance::all_management:archer::RxAcceptance::broadcast_bit_clear);
        });});});};
        if(std::string(argv[1])!="--run-owned-source-profile")archer::with_usb_rx_aggregation(bus,run_context);
        else run_context();
        measure("after_windows");
        if(session.phase()!=archer::ContextPhase::cleanup_acknowledged || (archer::channel_bus_read(bus,0x10398)&0x3ffff))throw std::runtime_error("Context cleanup/TX pause not confirmed");
        reservation.clean();
        std::cout<<"{\"result\":\"context_cleanup_acknowledged\",\"transfers\":"<<transfers<<",\"normal_packets\":"<<normal<<",\"beacons\":"<<beacons<<",\"network_tx_paused\":true,\"initial_macid_pause_measured\":false}"<<std::endl;
    });return 0;
 }catch(const std::exception& e){std::cerr<<"context-rx: "<<e.what()<<"; reservation may remain DIRTY; no replay\n";return 2;}
}
