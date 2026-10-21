#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/mac_reset.hpp"
#include "archer/rf_power.hpp"
#include "archer/bb_postinit.hpp"
#include "archer/rfk_protocol.hpp"
#include "archer/hfc_init.hpp"
#include "archer/dmac_init.hpp"
#include "boot_payload.hpp"
#include "radio_payload.hpp"
#include <fstream>
#include <filesystem>
static void delay_us(unsigned us){std::this_thread::sleep_for(std::chrono::microseconds(us));}
int main(int argc,char** argv){
 if(argc!=3||(std::string(argv[1])!="--cold-scc-radio" && std::string(argv[1])!="--powered-cold-scc-radio" && std::string(argv[1])!="--resume-loaded-scc-radio")){std::cout<<"Usage: archer-radio-init [--cold-scc-radio | --powered-cold-scc-radio | --resume-loaded-scc-radio] NEW_CAPTURE_DIRECTORY\n";return 2;}
 bool reset=false;
 try{
    archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
    if(!std::filesystem::create_directory(argv[2]))throw std::runtime_error("Capture directory must be new");
    auto images=trusted_boot_images();auto nctl=archer::selected_rf_nctl_plan(trusted_nctl_table);
    std::vector<unsigned> sequences{lock.reserve(),lock.reserve()};
    for(unsigned i=0;i<2;++i)(void)archer::radio_store_commands(trusted_radio_tables[i],sequences[i]);
    MacRegisterBus bus;MacBulkChannel usb;
    archer::with_wlan_io_window(bus,[&]{auto initialize=[&]{
        if(archer::read_chip_identity(bus).analog_id!=0x6933)throw std::runtime_error("Unexpected analog identity");
        if(std::string(argv[1])=="--resume-loaded-scc-radio") {
            if((bus.read(0x1e0,4)&0x0c00003f)!=0x0c000022 ||
               (bus.read(0x84,4)&0x300007)!=2)
                throw std::runtime_error("Loaded SCC resume prerequisites missing");
        } else {
            std::cout<<"{\"phase\":\"reset_to_scc\"}"<<std::endl;reset=true;
            archer::reset_to_usb2_scc(bus,delay_us);
            archer::enable_adda_power(bus);
            archer::boot_firmware(bus,images,[&](const auto& f){usb.send(f);},[](const char* p){std::cout<<"{\"phase\":\""<<p<<"\"}"<<std::endl;},true);
        }
        std::cout<<"{\"phase\":\"hfc_dmac\"}"<<std::endl;
        archer::enable_8922a_shared_cmac(bus,true);
        archer::enable_cmac0(bus);
        (void)archer::initialize_usb2_scc_hfc(bus,archer::usb2_scc_hfc_profile(),true,true,delay_us);
        const auto cut=archer::initialize_usb2_scc_dmac_remainder(bus,{true,true,true,archer::DmacReservedMpduInfo{2059,256,1,18}},[]{delay_us(1);});
        std::cout<<"{\"result\":\"dmac_cut_control\",\"requested\":554567152,\"effective\":"<<cut<<"}"<<std::endl;
        archer::enable_cmac0(bus);
        bus.write(0x84,1,bus.read(0x84,1)|3);bus.write(0x842c,4,0x7ff97ff9);
        for(auto [a,v]:trusted_bb_table)bus.write(a,4,v);
        archer::apply_bb_init_plan(bus,archer::rtl8922a_bb_postinit_plan(),delay_us);
        std::cout<<"{\"phase\":\"radio_tables\"}"<<std::endl;
        archer::initialize_radio_tables(bus,trusted_radio_tables,sequences,[&](const auto& f){usb.send(f);},delay_us,
            [](unsigned path,unsigned count){std::cout<<"{\"radio_path\":"<<path<<",\"entries\":"<<count<<"}"<<std::endl;});
        std::cout<<"{\"phase\":\"nctl\"}"<<std::endl;
        archer::apply_bb_init_plan(bus,archer::be_nctl_preinit_plan(),delay_us);
        (void)archer::check_be_nctl(bus,delay_us,100);
        archer::apply_bb_init_plan(bus,nctl,delay_us);
        archer::BeRfTransport rf(bus);
        for(unsigned path=0;path<2;++path)for(auto a:{0u,5u,0x18u,0x42u}) {
            const auto value=rf.read_full(path,a);
            std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":"<<a<<",\"value\":"<<value<<"}"<<std::endl;
        }
        std::cout<<"{\"result\":\"scc_radio_nctl_applied\",\"calibration_complete\":false}"<<std::endl;
    };
    if(std::string(argv[1])!="--cold-scc-radio") {
        for(const auto [a,v]:std::vector<std::pair<unsigned,unsigned>>{{0xe0,3},{0x90,0x6f},{0x80,0xeb},{0x81,0xeb},{0xe1,0x52}})
            if(archer::read_xtal_power_byte(bus,a)!=v)throw std::runtime_error("Powered profile mismatch before reset");
        initialize();
    } else archer::with_rf_power_window(bus,initialize,true);
    });
    return 0;
 }catch(const std::exception& e){std::cerr<<"radio-init: "<<e.what()<< (reset?"; reset transition started, recovery required":"")<<'\n';return 2;}
}
