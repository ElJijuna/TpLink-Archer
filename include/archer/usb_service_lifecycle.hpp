#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace archer {
inline constexpr std::array<std::uint8_t,9> service_endpoints{0x84,0x88,0x05,0x06,0x07,0x09,0x0a,0x0b,0x0c};
inline bool service_usb2_profile(std::uint16_t vendor,std::uint16_t product,
                                const std::uint8_t* data,std::size_t size) {
 if(vendor!=0x37ad||product!=0x0104||!data||size<9||size>4096||data[0]!=9||data[1]!=2||
    (unsigned(data[2])|(unsigned(data[3])<<8))!=size||data[4]!=1||data[5]!=1)return false;
 bool interface=false;unsigned seen=0;
 for(std::size_t at=9;at<size;){
  if(size-at<2||data[at]<2||data[at]>size-at)return false;
  const auto* d=data+at;
  if(d[1]==4){
   if(interface||d[0]!=9||d[2]!=0||d[3]!=0||d[4]!=9||d[5]!=255||d[6]!=255||d[7]!=255)return false;
   interface=true;
  }else if(d[1]==5){
   if(!interface||d[0]!=7||d[3]!=2||d[4]!=0||d[5]!=2||d[6]!=0)return false;
   unsigned bit=0;for(unsigned i=0;i<service_endpoints.size();++i)if(d[2]==service_endpoints[i])bit=1u<<i;
   if(!bit||(seen&bit))return false;seen|=bit;
  }else return false; // This initial service supports only the measured USB2 profile.
  at+=d[0];
 }
 return interface&&seen==0x1ff;
}
enum class UsbServiceState { idle,starting,ready,stopping,stopped };
// Serialized on the service's default dispatch queue. There are no async submits
// in this phase. A future transport must gate admission before starting a request.
template<class Backend> class UsbServiceLifecycle {
 Backend& backend_;UsbServiceState state_=UsbServiceState::idle;
 bool opened_=false;unsigned pipes_=0;
public:
 explicit UsbServiceLifecycle(Backend& backend):backend_(backend){}
 UsbServiceState state()const{return state_;}
 bool accepts_io()const{return state_==UsbServiceState::ready;}
 std::int32_t start(){
  if(state_!=UsbServiceState::idle)return Backend::invalid_state;
  state_=UsbServiceState::starting;
  auto status=backend_.open();
  if(!status){opened_=true;status=backend_.validate();}
  if(!status)for(unsigned i=0;i<service_endpoints.size();++i){
   status=backend_.copy_pipe(i,service_endpoints[i]);if(status)break;++pipes_;
  }
  if(status){stop();return status;}
  state_=UsbServiceState::ready;return 0;
 }
 std::int32_t stop(){
  if(state_==UsbServiceState::stopped||state_==UsbServiceState::stopping)return 0;
  state_=UsbServiceState::stopping; // Close admission BEFORE any abort callback.
  std::int32_t first=0;
  const auto keep=[&](std::int32_t error){if(!first&&error)first=error;};
  if(opened_){
   keep(backend_.abort_control());
   for(unsigned i=0;i<pipes_;++i)keep(backend_.abort_pipe(i));
   // Close is the interface-wide abort/drain boundary, even after Abort errors.
   keep(backend_.close());opened_=false;
  }
  while(pipes_)backend_.release_pipe(--pipes_);
  state_=UsbServiceState::stopped;return first;
 }
};
}
