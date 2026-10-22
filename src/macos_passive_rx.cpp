#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/rx_scan_window.hpp"
#include "archer/beacon.hpp"
#include "archer/rx_counters.hpp"
#include "archer/ple_probe.hpp"
#include "archer/phy_report_window.hpp"
#include "archer/physts_window.hpp"
#include "archer/port_rx_window.hpp"
#include <filesystem>
#include <fstream>
int main(int argc,char** argv){
 const std::vector<std::string> modes{"--receive-channel1","--receive-channel1-reset-counters",
    "--diagnostic-rmac-channel1","--diagnostic-bc-clear","--diagnostic-sniffer",
    "--diagnostic-response-engine","--diagnostic-phy-report","--diagnostic-physts","--diagnostic-port0"};
 if(argc!=3 || std::find(modes.begin(),modes.end(),argv[1])==modes.end()){
    std::cerr<<"Usage: archer-passive-rx MODE NEW_DIRECTORY; MODE:";
    for(const auto& mode:modes)std::cerr<<' '<<mode;
    std::cerr<<'\n';return 2;
 }
 try{
    const std::filesystem::path directory=argv[2];
    if(!std::filesystem::create_directory(directory))throw std::runtime_error("Capture directory must be new");
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
    MacRegisterBus bus;MacBulkChannel bulk;
    unsigned transfers=0,normal=0,beacons=0,probe=0,errors=0,unparsed=0;
    archer::with_wlan_io_window(bus,[&]{
        if((archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022 ||
           (archer::channel_bus_read(bus,0x84)&0x300007)!=7 ||
           (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9 ||
           !(archer::channel_bus_read(bus,0x9c00)&1))
            throw std::runtime_error("Loaded radio / appended FCS prerequisites absent");
        archer::BeRfTransport rf(bus);
        for(unsigned path=0;path<2;++path)
            if((rf.read_full(path,0x10018)&0x3ff)!=1)
                throw std::runtime_error("Digital RF channel1 prerequisite absent");
        const auto capture=[&]{archer::with_passive_rx_window(bus,[&]{
            const auto snapshot=[&](const char* phase){
                // Source RTL8922A PMAC map, BB aperture +0x20000; no clear/reset.
                for(unsigned a:{0x7880u,0x8400u,0x8404u,0x8914u,0x8938u,0x9058u,
                    0x10c00u,0x10c84u,0x10400u,0x11400u,0x11404u,0x11470u,0x11440u,0x20704u,0x20980u,0x20988u,0x20994u,0x209a4u,
                    0x20730u,0x20e24u,0x22e10u,0x22e14u,
                    0x20e64u,0x20e60u,0x20e5cu,0x20e58u}){
                    const auto v=archer::channel_bus_read(bus,a);
                    std::cout<<"{\"operation\":\"rx_snapshot\",\"phase\":\""<<phase
                             <<"\",\"address\":"<<a<<",\"value\":"<<v<<"}"<<std::endl;
                }
                const auto empty=archer::ple_debug_read(bus,7,0);
                const auto quota=archer::ple_debug_read(bus,1,6);
                std::cout<<"{\"operation\":\"ple_snapshot\",\"phase\":\""<<phase
                         <<"\",\"group0_empty\":"<<empty<<",\"quota6_raw\":"<<quota<<"}"<<std::endl;
                for(unsigned q=0;q<32;++q)if(!(empty&(1u<<q))){
                    const auto raw=archer::ple_debug_read(bus,6,q*2+1);
                    std::cout<<"{\"operation\":\"ple_queue\",\"phase\":\""<<phase
                             <<"\",\"queue\":"<<q<<",\"raw\":"<<raw
                             <<",\"packet_count\":"<<(raw&0xfff)<<"}"<<std::endl;
                }
            };
            snapshot("before");
            if(std::string(argv[1])!="--receive-channel1"){
                archer::reset_rx_diagnostic_counters(bus);
                snapshot("after_counter_reset");
            }
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(transfers<256 && std::chrono::steady_clock::now()<deadline){
                for(unsigned pipe=0;pipe<2 && transfers<256;++pipe){
                    auto bytes=bulk.receive(pipe,10);if(bytes.empty())continue;
                    std::ofstream file(directory/("rx-"+std::to_string(transfers++)+".bin"),std::ios::binary);
                    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.close();
                    if(!file)throw std::runtime_error("Capture write failed");
                    try{
                        for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
                            if(m.packet.packet_type!=0)continue;
                            ++normal;if(m.crc_error||m.icv_error){++errors;continue;}
                            try{const auto b=archer::decode_rx_beacon(bytes,m);if(b.probe_response)++probe;else ++beacons;}
                            catch(const std::invalid_argument&){++unparsed;}
                        }
                    }catch(const std::invalid_argument&){++unparsed;}
                }
            }
            snapshot("after");
        },std::string(argv[1])=="--diagnostic-rmac-channel1",
            std::string(argv[1])=="--diagnostic-bc-clear"?archer::RxAcceptance::broadcast_bit_clear:
            std::string(argv[1])=="--diagnostic-sniffer"?archer::RxAcceptance::sniffer:
            std::string(argv[1])=="--diagnostic-response-engine"?archer::RxAcceptance::response_engine:archer::RxAcceptance::broadcast);
        };
        if(std::string(argv[1])=="--diagnostic-port0")archer::with_unlinked_port0_window(bus,capture);
        else if(std::string(argv[1])=="--diagnostic-physts")
            archer::with_physts_window(bus,[&]{archer::with_phy_report_window(bus,capture);});
        else if(std::string(argv[1])=="--diagnostic-phy-report")archer::with_phy_report_window(bus,capture);
        else capture();
        if(archer::channel_bus_read(bus,0x10398)&0x3ffff)
            throw std::runtime_error("TX scheduler no longer paused");
    });
    std::cout<<"{\"result\":\"passive_rx_window_restored\",\"transfers\":"<<transfers
             <<",\"normal_packets\":"<<normal<<",\"beacons\":"<<beacons<<",\"probe_responses\":"<<probe
             <<",\"descriptor_errors\":"<<errors<<",\"unparsed\":"<<unparsed
             <<",\"network_tx_paused\":true}"<<std::endl;
    return 0;
 }catch(const std::exception& e){std::cerr<<"passive-rx: "<<e.what()<<'\n';return 2;}
}
