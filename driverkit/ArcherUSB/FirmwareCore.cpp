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
}
