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
 kern_return_t last_close_status=0;
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
 kern_return_t close(){last_close_status=interface->Close(owner,0);return last_close_status;}
 void release_pipe(unsigned i){pipes[i]->release();pipes[i]=nullptr;}
};
#include "USBTransport.inc"
#include "FirmwareCore.hpp"
struct ArcherUSB_IVars {
 USBBackend backend;
 archer::UsbServiceLifecycle<USBBackend> lifecycle;
 USBTransport transport;
 archer::DriverFirmwareEngine firmware;
 bool super_started=false,stop_requested=false,stop_held=false;
 IOService* stop_provider=nullptr;
 kern_return_t stop_error=0;
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
 if(!status)status=ivars->transport.initialize(this,&ivars->backend);
 if(status){
  ivars->transport.state.stop();
  ivars->transport.clear_buffers();
  ivars->lifecycle.stop();
  interface->release();ivars->backend.interface=nullptr;
  super::Stop(provider,SUPERDISPATCH);ivars->super_started=false;
 }
 // No RegisterEthernetInterface, firmware or automatic register/bulk submission.
 // Queues/supplicant and authenticated link state are later integration stages.
 return status;
}
kern_return_t IMPL(ArcherUSB,Stop){
 if(!ivars||ivars->stop_requested)return kIOReturnSuccess;
 ivars->stop_requested=true;ivars->stop_provider=provider;provider->retain();
 retain();ivars->stop_held=true;
 auto& t=ivars->transport;
 ivars->firmware.cancel();
 t.state.stop(); // Reject every new request/result BEFORE aborts and callbacks.
 ivars->stop_error=ivars->lifecycle.stop();
 t.closed=ivars->backend.last_close_status==kIOReturnSuccess;
 for(unsigned i=0;i<t.requests.size();++i)t.cancel_slot(i);
 FinishStopIfReady();
 return kIOReturnSuccess; // Super Stop is deferred until all Cancel barriers finish.
}
void ArcherUSB::FinishStopIfReady(){
 if(!ivars||!ivars->stop_requested||!ivars->stop_held)return;
 auto& t=ivars->transport;
 if(!t.closed||t.active_calls||t.control_busy||t.has_actions())return;
 t.clear_buffers();
 if(ivars->super_started){
  const auto status=super::Stop(ivars->stop_provider,SUPERDISPATCH);
  if(!ivars->stop_error)ivars->stop_error=status;
  ivars->super_started=false;
 }
 if(ivars->backend.interface){ivars->backend.interface->release();ivars->backend.interface=nullptr;}
 ivars->stop_provider->release();ivars->stop_provider=nullptr;
 ivars->stop_held=false;release();
}
void ArcherUSB::TransportCancelDone(std::uint32_t slot,std::uint64_t ticket){
 if(!ivars)return;auto& t=ivars->transport;
 if(!t.on_queue()||slot>=t.requests.size())return;
 auto& r=t.requests[slot];
 if(!r.action||!r.cancelling||r.token.ticket!=ticket||!t.state.cancel_confirmed(r.token))return;
 auto* action=r.action;r.action=nullptr;r.cancelling=false;r.token={};action->release();
 FinishStopIfReady();
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

static kern_return_t register_transfer(ArcherUSB* owner,USBTransport& t,bool write,
                                       std::uint32_t address,unsigned width,std::uint32_t& value){
 if(!t.on_queue()||!t.state.running()||t.fault)return kIOReturnNotReady;
 if(t.control_busy)return kIOReturnBusy;
 archer::DriverRegisterSetup setup;
 if(!archer::driver_register_setup(write,address,width,setup))return kIOReturnBadArgument;
 TransportCallHold hold(owner,t.active_calls);t.control_busy=true;
 auto& buffer=write?t.control_write:t.control_read;
 std::memset(buffer.bytes,0,4);
 if(write)for(unsigned i=0;i<width;++i)buffer.bytes[i]=std::uint8_t(value>>(i*8));
 std::uint16_t actual=0;
 auto status=t.backend->interface->DeviceRequest(setup.type,setup.request,setup.value,setup.index,
                                               setup.length,buffer.memory,&actual,200);
 t.control_busy=false;
 if(!t.state.running())return kIOReturnAborted;
 std::uint32_t observed=0;
 const auto completion=archer::driver_register_finish(width,status,actual,buffer.bytes,observed);
 if(completion==archer::RegisterCompletion::usb_error)return status;
 if(completion==archer::RegisterCompletion::overrun)return kIOReturnOverrun;
 if(completion==archer::RegisterCompletion::short_io)return kIOReturnUnderrun;
 if(completion!=archer::RegisterCompletion::ok)return kIOReturnBadArgument;
 if(!write)value=observed;
 return 0;
}
kern_return_t ArcherUSB::ReadRegister(std::uint32_t address,std::uint32_t width,std::uint32_t* value){
 if(!ivars||!value)return kIOReturnBadArgument;
 std::uint32_t result=0;const auto status=register_transfer(this,ivars->transport,false,address,width,result);
 if(!status)*value=result;return status;
}
kern_return_t ArcherUSB::WriteRegister(std::uint32_t address,std::uint32_t width,std::uint32_t value){
 if(!ivars)return kIOReturnNotReady;return register_transfer(this,ivars->transport,true,address,width,value);
}
kern_return_t ArcherUSB::SubmitBulk(std::uint8_t endpoint,const std::uint8_t* data,
 std::uint32_t length,std::uint32_t timeoutMS,std::uint64_t* ticket){
 if(!ivars||!ticket)return kIOReturnBadArgument;*ticket=0;
 auto& t=ivars->transport;
 if(!t.on_queue()||!t.state.running()||!ivars->lifecycle.accepts_io()||t.fault)return kIOReturnNotReady;
 unsigned pipe=archer::service_endpoints.size();
 for(unsigned i=0;i<archer::service_endpoints.size();++i)if(archer::service_endpoints[i]==endpoint)pipe=i;
 if(pipe==archer::service_endpoints.size()||!length||length>archer::usb_transport_capacity||!timeoutMS||timeoutMS>2000||(!(endpoint&0x80)&&!data))return kIOReturnBadArgument;
 TransportCallHold hold(this,t.active_calls);
 const auto token=t.state.reserve(endpoint,length,timeoutMS);if(!token.ticket)return kIOReturnNoResources;
 auto& r=t.requests[token.slot];r.token=token;
 auto status=CreateActionBulkComplete(sizeof(archer::TransferToken),&r.action);
 if(!status&&!r.action)status=kIOReturnNoMemory;
 if(status){
  // Failed creation owns no submitted I/O. A partial action still needs Cancel.
  if(r.action)t.cancel_slot(token.slot);
  else{t.state.begin_cancel(token.ticket);t.state.cancel_confirmed(token);r.token={};}
  return status;
 }
 auto* context=static_cast<archer::TransferToken*>(r.action->GetReference());
 if(!context){t.cancel_slot(token.slot);return kIOReturnNoMemory;}
 *context=token;
 if(endpoint&0x80)std::memset(r.buffer.bytes,0,length);
 else std::memmove(r.buffer.bytes,data,length);
 status=t.backend->pipes[pipe]->AsyncIO(r.buffer.memory,length,r.action,timeoutMS);
 if(status){t.cancel_slot(token.slot);return status;} // Never resubmit an uncertain OUT.
 if(!t.state.running())return kIOReturnAborted;
 *ticket=token.ticket;return 0;
}
void IMPL(ArcherUSB,BulkComplete){
 (void)completionTimestamp;
 if(!ivars||!action)return;
 auto& t=ivars->transport;
 // OSAction dispatch normally uses Default. Marshal explicitly if another queue delivers it.
 if(!t.on_queue()){
  auto* queue=t.queue;if(!queue)return;retain();action->retain();
  queue->DispatchAsync(^{BulkComplete_Impl(action,status,actualByteCount,completionTimestamp);action->release();release();});return;
 }
 const auto* token=static_cast<const archer::TransferToken*>(action->GetReference());
 if(!token||token->slot>=t.requests.size()||t.requests[token->slot].action!=action)return;
 t.state.complete(*token,status,actualByteCount); // No parsing/rearm or plaintext exposure here.
}
kern_return_t ArcherUSB::CopyBulkResult(std::uint64_t ticket,std::int32_t* status,
 std::uint8_t* output,std::uint32_t capacity,std::uint32_t* actual){
 if(!ivars||!status||!actual)return kIOReturnBadArgument;auto& t=ivars->transport;
 if(!t.on_queue()||!t.state.running())return kIOReturnNotReady;
 const auto i=t.state.index(ticket);const auto* slot=t.state.slot(i);
 if(!slot)return kIOReturnBadArgument;
 if(slot->phase!=archer::TransferPhase::complete)return kIOReturnNotReady;
 auto result=slot->status;
 if(slot->kind==archer::TransferKind::overrun)result=kIOReturnOverrun;
 if(slot->kind==archer::TransferKind::short_write)result=kIOReturnUnderrun;
 if(!result&&(slot->endpoint&0x80)&&slot->actual){
  if(!output||capacity<slot->actual)return kIOReturnNoSpace;
  std::memmove(output,t.requests[i].buffer.bytes,slot->actual);
 }
 *status=result;*actual=slot->actual;return 0;
}
kern_return_t ArcherUSB::ReleaseBulk(std::uint64_t ticket){
 if(!ivars)return kIOReturnNotReady;auto& t=ivars->transport;
 if(!t.on_queue()||!t.state.running())return kIOReturnNotReady;
 const auto i=t.state.index(ticket);const auto* slot=t.state.slot(i);
 if(!slot)return kIOReturnBadArgument;
 if(slot->phase!=archer::TransferPhase::complete)return kIOReturnNotReady;
 TransportCallHold hold(this,t.active_calls);
 return t.cancel_slot(i);
}

kern_return_t ArcherUSB::GetTransportStatus(std::uint32_t* pending,std::int32_t* fault,bool* stopping){
 if(!ivars||!pending||!fault||!stopping)return kIOReturnBadArgument;
 const auto& t=ivars->transport;if(!t.on_queue())return kIOReturnNotPermitted;
 *pending=t.state.active();*fault=t.fault?t.fault:ivars->stop_error;*stopping=ivars->stop_requested;return 0;
}

#include "archer/driver_boot_controls.hpp"
kern_return_t ArcherUSB::ReadBootControls(std::uint32_t* values,std::uint32_t capacity,std::uint32_t* classification){
 if(!ivars||!values||!classification||capacity!=archer::driver_boot_registers.size())return kIOReturnBadArgument;
 auto& t=ivars->transport;if(!t.on_queue()||!t.state.running()||t.fault)return kIOReturnNotReady;
 TransportCallHold hold(this,t.active_calls);
 std::array<std::uint32_t,6> observed{};
 for(unsigned i=0;i<observed.size();++i){
  const auto status=ReadRegister(archer::driver_boot_registers[i],4,&observed[i]);if(status)return status;
 }
 if(!t.state.running())return kIOReturnAborted;
 const auto decoded=archer::classify_driver_boot_controls(observed);
 if(decoded==archer::DriverBootControls::inaccessible)return kIOReturnNotReadable;
 std::copy(observed.begin(),observed.end(),values);*classification=std::uint32_t(decoded);
 return 0; // No writes/reset/download; full boot ownership/images/window gates still required.
}

kern_return_t ArcherUSB::GetBoundFirmwarePlan(std::uint32_t* images,std::uint32_t* frames,std::uint32_t* bytes){
 if(!ivars||!images||!frames||!bytes)return kIOReturnBadArgument;
 if(!ivars->transport.on_queue())return kIOReturnNotPermitted;
 const auto& images_view=archer::driverkit::bound_firmware_images();
 if(!archer::DriverFirmwareEngine::valid_images(images_view))return kIOReturnUnsupported;
 std::uint32_t count=0,total=0;
 for(const auto& image:images_view){count+=image.count;for(unsigned i=0;i<image.count;++i)total+=image.frames[i].size;}
 *images=2;*frames=count;*bytes=total;return 0;
 // Metadata only. No engine.begin() until ownership/windows/drain orchestration is ported.
}
