#include "archer/firmware_state.hpp"
#include <cassert>
#include <iostream>
int main(){
    constexpr auto s=archer::decode_firmware_state(0x0c000000,0x80f30000,1);
    static_assert(s.status==3&&!s.bb0_download_enabled&&!s.bb0_boot_ready);
    static_assert(!s.bb0_ip_reset_released&&!s.bb0_platform_reset_released&&!s.bb0_memory_sleep);
    for(unsigned bit=9;bit<=12;++bit){
        const auto x=archer::decode_firmware_state(1u<<bit,0,0);
        assert(x.wlan_download_enabled==(bit==9));assert(x.data_download_enabled==(bit==10));
        assert(x.bb0_download_enabled==(bit==11));assert(x.bb1_download_enabled==(bit==12));
    }
    constexpr auto all=archer::decode_firmware_state(0xffffffff,7,1u<<17);
    static_assert(all.status==15&&all.run_environment==3&&all.bb0_boot_ready&&all.bb0_memory_sleep);
    std::cout<<"Firmware controls decode without inferring image identity: OK\n";
}
