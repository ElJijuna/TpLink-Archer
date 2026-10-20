#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/firmware_state.hpp"
int main(int argc,char**){
    if(argc!=1){std::cerr<<"Usage: archer-firmware-state (read-only, project root)\n";return 2;}
    try {
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        if((bus.read(0xfc,4)&255)!=0x71)throw std::runtime_error("Unexpected chip");
        const auto fw=bus.read(0x1e0,4),fen=bus.read(0x84,4),mem=bus.read(0xd0,4);
        for(auto value:{fw,fen,mem})if(value==0xeaeaeaea||value==0xdeadbeef)throw std::runtime_error("Firmware control inaccessible");
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
