#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/firmware_state.hpp"
#include "archer/xtal_identity.hpp"
int main(int argc,char** argv){
    const bool access=argc==2&&std::string(argv[1])=="--inspect-boot-access";
    if(argc!=1&&!access){std::cerr<<"Usage: archer-firmware-state [--inspect-boot-access] (project root)\n";return 2;}
    try {
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        if((bus.read(0xfc,4)&255)!=0x71)throw std::runtime_error("Unexpected chip");
        const auto fw=bus.read(0x1e0,4),fen=bus.read(0x84,4),mem=bus.read(0xd0,4);
        for(auto value:{fw,fen,mem})if(value==0xeaeaeaea||value==0xdeadbeef)throw std::runtime_error("Firmware control inaccessible");
        if(access){
            archer::with_wlan_io_window(bus,[&]{
                (void)bus.read(0,2);(void)bus.read(0xcc,1);
                for(auto offset:{0x2du,0x2eu,0x80u,0x81u,0x90u,0xa1u,0xe0u,0xe1u}){
                    const auto value=archer::read_xtal_power_byte(bus,offset);
                    std::cout<<"{\"result\":\"xtal_power_read\",\"offset\":"<<offset<<",\"value\":"<<value<<"}"<<std::endl;
                }
                for(auto width:{1u,2u,4u})(void)bus.read(0x888,width);
                for(auto address:{0x88u,0x7880u,0xb000u,0xb010u,0x8420u,0x8400u,0x8404u})(void)bus.read(address,4);
            });
        }
        const auto s=archer::decode_firmware_state(fw,fen,mem);
        std::cout<<"{\"result\":\"firmware_controls\",\"status\":"<<s.status
            <<",\"run_environment\":"<<s.run_environment
            <<",\"wlan_download_enabled\":"<<s.wlan_download_enabled
            <<",\"data_download_enabled\":"<<s.data_download_enabled
            <<",\"bb0_download_enabled\":"<<s.bb0_download_enabled
            <<",\"bb1_download_enabled\":"<<s.bb1_download_enabled
            <<",\"bb0_boot_ready\":"<<s.bb0_boot_ready
            <<",\"bb0_ip_reset_released\":"<<s.bb0_ip_reset_released
            <<",\"bb0_platform_reset_released\":"<<s.bb0_platform_reset_released
            <<",\"bb0_memory_sleep\":"<<s.bb0_memory_sleep
            <<",\"resident_version\":null,\"bbmcu_liveness_proven\":false}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"firmware-state: "<<e.what()<<'\n';return 2;}
}
