#include "FirmwareCore.hpp"
#include "BoundFirmware.hpp"
namespace archer::driverkit {
const std::array<DriverFirmwareImage,2>& bound_firmware_images(){return driver_bound_firmware;}
bool start_bound_firmware(DriverFirmwareEngine& engine,std::uint64_t epoch,std::uint64_t now_us){
 return engine.begin(driver_bound_firmware,epoch,now_us);
}
DriverFirmwareAction next_firmware_action(DriverFirmwareEngine& engine,std::uint64_t now_us){return engine.next(now_us);}
bool complete_firmware_action(DriverFirmwareEngine& engine,const DriverFirmwareAction& action,
                             std::int32_t status,std::uint32_t value,std::uint32_t actual,std::uint64_t now_us){
 return engine.complete(action,status,value,actual,now_us);
}

bool start_power_windows(DriverPowerWindows& windows,std::uint64_t epoch,std::uint64_t now_us){return windows.begin(epoch,now_us,true);}
DriverFirmwareAction next_power_window_action(DriverPowerWindows& windows,std::uint64_t now_us){return windows.next(now_us);}
bool complete_power_window_action(DriverPowerWindows& windows,const DriverFirmwareAction& action,
                                 std::int32_t status,std::uint32_t value,std::uint32_t actual,std::uint64_t now_us){
 return windows.complete(action,status,value,actual,now_us);
}
bool restore_power_windows(DriverPowerWindows& windows,std::uint64_t now_us,bool body_succeeded){return windows.close(now_us,body_succeeded);}

bool restore_rf_windows(DriverPowerWindows& windows,std::uint64_t now_us,bool body_succeeded){return windows.close_rf(now_us,body_succeeded);}
bool finish_wlan_window(DriverPowerWindows& windows,std::uint64_t now_us){return windows.finish_wlan(now_us);}
}
