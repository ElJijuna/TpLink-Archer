#pragma once
#include <array>
#include <cstdint>
#include "archer/firmware_state.hpp"
namespace archer {
inline constexpr std::array<std::uint32_t,6> driver_boot_registers{0xfc,0xf0,0x1e0,0x84,0xd0,0x842c};
enum class DriverBootControls : std::uint32_t {
 inaccessible=0,unsupported_chip_cut=1,cold_controls_only=2,running_controls_only=3,unknown=4
};
// Observation only: neither firmware identity/liveness nor permission to reset.
constexpr DriverBootControls classify_driver_boot_controls(const std::array<std::uint32_t,6>& v){
 for(const auto value:v)if(value==0xeaeaeaea||value==0xdeadbeef)return DriverBootControls::inaccessible;
 if((v[0]&255)!=0x71||((v[1]>>12)&15)!=1)return DriverBootControls::unsupported_chip_cut;
 const auto state=decode_firmware_state(v[2],v[3],v[4]);
 if(state.status!=3||state.run_environment!=0)return DriverBootControls::unknown;
 if(!(v[3]&0x33)&&v[5]==0)return DriverBootControls::cold_controls_only;
 if(state.status==3&&state.run_environment==0&&(v[2]&0x3f)==0x22&&(v[3]&7)==7&&!state.bb0_memory_sleep)
  return DriverBootControls::running_controls_only;
 return DriverBootControls::unknown;
}
}
