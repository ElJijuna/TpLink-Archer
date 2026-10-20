#pragma once
#include <cstdint>
namespace archer {
struct FirmwareState {
    unsigned status,run_environment;
    bool wlan_download_enabled,data_download_enabled,bb0_download_enabled,bb1_download_enabled;
    bool bb0_boot_ready,bb0_ip_reset_released,bb0_platform_reset_released,bb0_memory_sleep;
};
// These are control/status bits, not a firmware identity or BBMCU liveness test.
constexpr FirmwareState decode_firmware_state(std::uint32_t fw,std::uint32_t fen,std::uint32_t mem) {
    return {(fw>>26)&15,fw>>30,bool(fw&(1u<<9)),bool(fw&(1u<<10)),
        bool(fw&(1u<<11)),bool(fw&(1u<<12)),bool(fen&4),bool(fen&2),bool(fen&1),bool(mem&(1u<<17))};
}
}
