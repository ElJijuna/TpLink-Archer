#include "ArcherUSB.h"
#include <DriverKit/IOLib.h>
#include <USBDriverKit/IOUSBHostDevice.h>
#include <USBDriverKit/IOUSBHostPipe.h>
#include <USBDriverKit/USBDriverKitDefs.h>
#include "archer/usb_service_lifecycle.hpp"
#include <new>

struct USBBackend {
 static constexpr std::int32_t invalid_state=kIOReturnNotPermitted;
 ArcherUSB* owner=nullptr;
 IOUSBHostInterface* interface=nullptr;
 std::array<IOUSBHostPipe*,9> pipes{};
 kern_return_t open(){return interface->Open(owner,0,nullptr);}
 kern_return_t validate(){
  IOUSBHostDevice* device=nullptr;
  auto status=interface->CopyDevice(&device);
  if(status||!device)return status?status:kIOReturnNoDevice;
  const auto* descriptor=device->CopyDeviceDescriptor();
  const auto* config=interface->CopyConfigurationDescriptor();
  bool valid=false;
  if(descriptor&&config){
   const auto* selected=interface->GetInterfaceDescriptor(config);
   const auto* d=reinterpret_cast<const std::uint8_t*>(descriptor);
   const auto* c=reinterpret_cast<const std::uint8_t*>(config);
   const std::size_t length=unsigned(c[2])|(unsigned(c[3])<<8);
   valid=descriptor->bLength==18&&descriptor->bDescriptorType==1&&selected&&
     selected->bInterfaceNumber==0&&selected->bAlternateSetting==0&&
     archer::service_usb2_profile(std::uint16_t(unsigned(d[8])|(unsigned(d[9])<<8)),
                                 std::uint16_t(unsigned(d[10])|(unsigned(d[11])<<8)),c,length);
  }
  if(config)IOUSBHostFreeDescriptor(config);
  if(descriptor)IOUSBHostFreeDescriptor(descriptor);
  device->release();return valid?kIOReturnSuccess:kIOReturnUnsupported;
 }
 kern_return_t copy_pipe(unsigned index,std::uint8_t endpoint){
  auto status=interface->CopyPipe(endpoint,&pipes[index]);
  if(status||!pipes[index]){if(pipes[index]){pipes[index]->release();pipes[index]=nullptr;}return status?status:kIOReturnNoDevice;}
  IOUSBStandardEndpointDescriptors descriptors{};
  status=pipes[index]->GetDescriptors(&descriptors,kIOUSBGetEndpointDescriptorOriginal);
  const auto& d=descriptors.descriptor;
  if(!status&&(d.bEndpointAddress!=endpoint||d.bmAttributes!=2||d.wMaxPacketSize!=512))status=kIOReturnUnsupported;
  if(status){pipes[index]->release();pipes[index]=nullptr;}return status;
 }
 kern_return_t abort_control(){return interface->AbortDeviceRequests(0,kIOReturnAborted);}
 kern_return_t abort_pipe(unsigned i){return pipes[i]->Abort(kIOUSBAbortSynchronous,kIOReturnAborted,nullptr);}
 kern_return_t close(){return interface->Close(owner,0);}
 void release_pipe(unsigned i){pipes[i]->release();pipes[i]=nullptr;}
};
struct ArcherUSB_IVars {
 USBBackend backend;
 archer::UsbServiceLifecycle<USBBackend> lifecycle;
 bool super_started=false;
 ArcherUSB_IVars():lifecycle(backend){}
};
#define super IOUserNetworkEthernet
bool ArcherUSB::init(){
 ivars=nullptr;
 if(!super::init())return false;
 auto* memory=IOMalloc(sizeof(ArcherUSB_IVars));if(!memory)return false;
 ivars=new(memory) ArcherUSB_IVars;ivars->backend.owner=this;return true;
}
void ArcherUSB::free(){
 if(ivars){
  // Stop owns teardown. Do not initiate blocking RPC/USB operations from free.
  ivars->~ArcherUSB_IVars();IOFree(ivars,sizeof(ArcherUSB_IVars));ivars=nullptr;
 }
 super::free();
}
kern_return_t IMPL(ArcherUSB,Start){
 if(!ivars||ivars->lifecycle.state()!=archer::UsbServiceState::idle)return kIOReturnNotPermitted;
 auto* interface=OSDynamicCast(IOUSBHostInterface,provider);if(!interface)return kIOReturnBadArgument;
 auto status=super::Start(provider,SUPERDISPATCH);if(status)return status;
 ivars->super_started=true;interface->retain();ivars->backend.interface=interface;
 status=ivars->lifecycle.start();
 if(status){
  interface->release();ivars->backend.interface=nullptr;
  super::Stop(provider,SUPERDISPATCH);ivars->super_started=false;
 }
 // Deliberately no RegisterEthernetInterface, RegisterService, firmware or I/O.
 // Queues/supplicant and authenticated link state are later integration stages.
 return status;
}
kern_return_t IMPL(ArcherUSB,Stop){
 if(!ivars||ivars->lifecycle.state()==archer::UsbServiceState::stopping)return kIOReturnSuccess;
 auto status=ivars->lifecycle.stop();
 if(ivars->backend.interface){ivars->backend.interface->release();ivars->backend.interface=nullptr;}
 if(ivars->super_started){auto parent=super::Stop(provider,SUPERDISPATCH);ivars->super_started=false;if(!status)status=parent;}
 return status;
}
kern_return_t IMPL(ArcherUSB,SetInterfaceEnable){return isEnable?kIOReturnNotReady:kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,SetPromiscuousModeEnable){return enable?kIOReturnUnsupported:kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,SetMulticastAddresses){(void)addresses;(void)count;return kIOReturnUnsupported;}
kern_return_t IMPL(ArcherUSB,SetAllMulticastModeEnable){return enable?kIOReturnUnsupported:kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,SelectMediaType){(void)mediaType;return kIOReturnUnsupported;}
kern_return_t IMPL(ArcherUSB,SetWakeOnMagicPacketEnable){return enable?kIOReturnUnsupported:kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,SetMTU){return mtu==1500?kIOReturnSuccess:kIOReturnUnsupported;}
kern_return_t IMPL(ArcherUSB,GetMaxTransferUnit){if(!mtu)return kIOReturnBadArgument;*mtu=1500;return kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,SetHardwareAssists){return hardwareAssists?kIOReturnUnsupported:kIOReturnSuccess;}
kern_return_t IMPL(ArcherUSB,GetHardwareAssists){if(!hardwareAssists)return kIOReturnBadArgument;*hardwareAssists=0;return kIOReturnSuccess;}
