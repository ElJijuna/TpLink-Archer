#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/cmac_init.hpp"
#include "archer/channel_bus.hpp"
int main(int argc,char** argv){
 if(argc!=2 || std::string(argv[1])!="--initialize-cmac0") {
    std::cerr<<"Usage: archer-cmac-init --initialize-cmac0\n";return 2;
 }
 try {
    archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");
    MacRegisterBus bus;
    archer::with_wlan_io_window(bus,[&]{
        if((archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022 ||
           (archer::channel_bus_read(bus,0x84)&0x300007)!=7 ||
           (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9 ||
           (archer::channel_bus_read(bus,0x9008)&0x1fff0000)!=(2059u<<16))
            throw std::runtime_error("Loaded SCC radio prerequisites absent");
        (void)archer::pause_cmac0_scheduler(bus);
        archer::initialize_usb2_scc_cmac0(bus,{true},[]{std::this_thread::sleep_for(std::chrono::microseconds(1));});
        if(archer::channel_bus_read(bus,0x10398)&0x3ffff)
            throw std::runtime_error("CMAC scheduler unexpectedly resumed");
        std::cout<<"{\"result\":\"cmac0_init_sequence_completed\",\"network_tx_paused\":true}"<<std::endl;
    });
    return 0;
 }catch(const std::exception& e){std::cerr<<"cmac-init: "<<e.what()<<"; keep TX paused, no calibration submitted\n";return 2;}
}
