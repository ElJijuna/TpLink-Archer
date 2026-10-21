#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/efuse_reader.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/rf_transport.hpp"
#include "archer/rf_power.hpp"
#include "boot_payload.hpp"
#include "archer/macos_bulk.hpp"
using BootUsb=MacBulkChannel;
int main(int argc,char** argv){
    const bool verify_loaded=argc==2&&std::string(argv[1])=="--verify-loaded-rf";
    const bool usb_no_dcpu=argc==2&&std::string(argv[1])=="--boot-usb-no-dcpu";
    if(argc!=2||(!verify_loaded&&!usb_no_dcpu&&std::string(argv[1])!="--boot-and-read-rf")){
        std::cout<<"Usage: archer-firmware-boot --boot-and-read-rf | --boot-usb-no-dcpu | --verify-loaded-rf (one attempt, project root)\n";return 2;
    }
    bool reset_started=false;
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        MacRegisterBus bus;
        if(verify_loaded){
            archer::with_wlan_io_window(bus,[&]{
                if((bus.read(0x1e0,4)&0xa00)||(bus.read(0x84,4)&7)!=7)throw std::runtime_error("Loaded firmware/BBMCU prerequisites missing");
                archer::with_rf_power_window(bus,[&]{
                    archer::BeRfTransport rf(bus);
                    for(unsigned path=0;path<2;++path)for(auto address:{0u,5u,0x18u,0x42u}){
                        auto value=rf.read_full(path,address);
                        std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":"<<address<<",\"value\":"<<value<<"}"<<std::endl;
                    }
                });
            });return 0;
        }
        const auto images=trusted_boot_images();BootUsb usb;
        archer::with_wlan_io_window(bus,[&]{
            auto run=[&]{
            if(archer::read_chip_identity(bus).analog_id!=0x6933)throw std::runtime_error("Unexpected analog identity");
            const auto security=archer::read_ddv_efuse(bus,archer::EfuseProfile::firmware_security).security;
            if(security!=std::vector<std::uint8_t>(4,255))throw std::runtime_error("Unsupported firmware security profile");
            archer::boot_firmware(bus,images,[&](const auto& frame){usb.send(frame);},[&](const char* phase){
                if(std::string(phase)=="cpu_reset")reset_started=true;
                std::cout<<"{\"phase\":\""<<phase<<"\"}"<<std::endl;
            },usb_no_dcpu);
            // RTL8922A enable_bb_rf; selected BB table, then the reset/clock portion of postinit.
            bus.write(0x84,1,bus.read(0x84,1)|3);
            bus.write(0x842c,4,0x7ff97ff9);
            for(auto [address,value]:trusted_bb_table)bus.write(address,4,value);
            bus.write(0x84,4,bus.read(0x84,4)|5);
            bus.write(0x20000,4,bus.read(0x20000,4)|4);
            bus.pause();
            archer::BeRfTransport rf(bus);
            for(unsigned path=0;path<2;++path){
                auto value=rf.read_full(path,0);
                std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":0,\"value\":"<<value<<"}"<<std::endl;
            }
            };
            if(usb_no_dcpu)archer::with_rf_power_window(bus,run);else run();
        });return 0;
    }catch(const std::exception& e){std::cerr<<"firmware-boot: "<<e.what()<<(reset_started?"; after CPU reset, reconnect before another attempt":"; stopped before CPU reset")<<"\n";return 2;}
}
