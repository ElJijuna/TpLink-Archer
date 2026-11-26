// Included after USBBackend; all mutable transport state is confined to Default.
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/OSAction.h>
#include "archer/usb_transport_state.hpp"
#include <atomic>
#include <cstring>

static std::atomic<std::uint64_t> transport_epoch_counter{1};
static std::uint64_t next_transport_epoch(){
 auto current=transport_epoch_counter.load();
 while(current!=std::numeric_limits<std::uint64_t>::max())
  if(transport_epoch_counter.compare_exchange_weak(current,current+1))return current;
 return 0;
}
struct TransportBuffer {
 IOBufferMemoryDescriptor* memory=nullptr;std::uint8_t* bytes=nullptr;std::size_t capacity=0;
 kern_return_t create(IOUSBHostInterface* interface,IOOptionBits direction,std::size_t length){
  auto status=interface->CreateIOBuffer(direction,length,&memory);
  if(status||!memory)return status?status:kIOReturnNoMemory;
  status=memory->SetLength(length);if(status)return status;
  IOAddressSegment range{};status=memory->GetAddressRange(&range);
  if(status||!range.address||range.length<length)return status?status:kIOReturnNoMemory;
  bytes=reinterpret_cast<std::uint8_t*>(range.address);capacity=length;std::memset(bytes,0,length);return 0;
 }
 void clear(){
  if(bytes){volatile std::uint8_t* p=bytes;for(std::size_t n=0;n<capacity;++n)p[n]=0;}
  if(memory)memory->release();memory=nullptr;bytes=nullptr;capacity=0;
 }
};
struct TransportRequest {
 TransportBuffer buffer;OSAction* action=nullptr;archer::TransferToken token{};bool cancelling=false;
};
struct USBTransport {
 ArcherUSB* owner=nullptr;USBBackend* backend=nullptr;IODispatchQueue* queue=nullptr;
 archer::UsbTransportState state;
 std::array<TransportRequest,archer::usb_transport_slots> requests{};
 TransportBuffer control_read,control_write;
 std::uint32_t active_calls=0;bool control_busy=false,closed=false;
 kern_return_t fault=0;
 bool on_queue()const{return queue&&queue->OnQueue();}
 kern_return_t initialize(ArcherUSB* service,USBBackend* usb){
  owner=service;backend=usb;
  auto status=owner->CopyDispatchQueue(kIOServiceDefaultQueueName,&queue);if(status)return status;
  for(auto& r:requests){status=r.buffer.create(backend->interface,kIOMemoryDirectionInOut,archer::usb_transport_capacity);if(status)return status;}
  status=control_read.create(backend->interface,kIOMemoryDirectionIn,4);if(status)return status;
  status=control_write.create(backend->interface,kIOMemoryDirectionOut,4);if(status)return status;
  const auto epoch=next_transport_epoch();return epoch&&state.activate(epoch)?0:kIOReturnNoResources;
 }
 bool has_actions()const{for(const auto& r:requests)if(r.action)return true;return false;}
 void clear_buffers(){
  // Only after no action callbacks/control calls remain and DMA close is proven,
  // or initialization failed before any request could have been submitted.
  for(auto& r:requests)r.buffer.clear();control_read.clear();control_write.clear();
  if(queue)queue->release();queue=nullptr;
 }
 kern_return_t cancel_slot(unsigned i){
  auto& r=requests[i];if(!r.action||r.cancelling)return 0;
  state.begin_cancel(r.token.ticket);r.cancelling=true;
  const auto ticket=r.token.ticket;auto* service=owner;auto* dispatch=queue;
  service->retain();
  const auto status=r.action->Cancel(^{
   dispatch->DispatchAsync(^{service->TransportCancelDone(i,ticket);service->release();});
  });
  if(status){
   // No forced free or retry: keep action/buffer/owner until a real Cancel ack.
   fault=status;state.stop();
  }
  return status;
 }
};
struct TransportCallHold {
 ArcherUSB* owner;std::uint32_t& count;
 TransportCallHold(ArcherUSB* service,std::uint32_t& active):owner(service),count(active){++count;}
 ~TransportCallHold(){--count;owner->FinishStopIfReady();}
};
