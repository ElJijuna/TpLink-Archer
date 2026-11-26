#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include "archer/usb_service_lifecycle.hpp"
namespace archer {
struct DriverRegisterSetup { std::uint8_t type=0,request=5;std::uint16_t value=0,index=0,length=0; };
inline bool driver_register_setup(bool write,std::uint32_t address,unsigned width,DriverRegisterSetup& out){
 if((width!=1&&width!=2&&width!=4)||address>0xffffff||width-1>0xffffff-address)return false;
 out={std::uint8_t(write?0x40:0xc0),5,std::uint16_t(address),std::uint16_t(address>>16),std::uint16_t(width)};return true;
}
enum class RegisterCompletion { ok,usb_error,short_io,overrun,bad_argument };
inline RegisterCompletion driver_register_finish(unsigned width,std::int32_t status,unsigned actual,
                                                 const std::uint8_t* bytes,std::uint32_t& value){
 if((width!=1&&width!=2&&width!=4)||!bytes)return RegisterCompletion::bad_argument;
 if(status)return RegisterCompletion::usb_error;
 if(actual!=width)return actual>width?RegisterCompletion::overrun:RegisterCompletion::short_io;
 std::uint32_t result=0;for(unsigned i=0;i<width;++i)result|=std::uint32_t(bytes[i])<<(8*i);
 value=result;return RegisterCompletion::ok;
}
inline constexpr unsigned usb_transport_slots=12,usb_transport_capacity=16384;
enum class TransferPhase { free,pending,complete,cancelling };
enum class TransferKind { ok,usb_error,short_write,overrun };
struct TransferSlot {
 TransferPhase phase=TransferPhase::free;TransferKind kind=TransferKind::ok;
 std::uint64_t ticket=0;std::uint8_t endpoint=0;std::uint32_t requested=0,actual=0;
 std::int32_t status=0;
};
struct TransferToken { std::uint64_t epoch=0,ticket=0;unsigned slot=usb_transport_slots; };
class UsbTransportState {
 std::array<TransferSlot,usb_transport_slots> slots_{};
 std::uint64_t epoch_=0,next_=1;bool running_=false;
public:
 explicit UsbTransportState(std::uint64_t first_ticket=1):next_(first_ticket){}
 bool activate(std::uint64_t epoch){
  if(running_||!epoch||epoch_||active())return false;epoch_=epoch;running_=true;return true;
 }
 bool running()const{return running_;}
 unsigned active()const{unsigned n=0;for(const auto& s:slots_)n+=s.phase!=TransferPhase::free;return n;}
 std::uint64_t epoch()const{return epoch_;}
 const TransferSlot* slot(unsigned i)const{return i<slots_.size()?&slots_[i]:nullptr;}
 const TransferSlot* find(std::uint64_t ticket)const{if(!ticket)return nullptr;for(const auto& s:slots_)if(s.phase!=TransferPhase::free&&s.ticket==ticket)return &s;return nullptr;}
 unsigned index(std::uint64_t ticket)const{for(unsigned i=0;i<slots_.size();++i)if(ticket&&slots_[i].phase!=TransferPhase::free&&slots_[i].ticket==ticket)return i;return usb_transport_slots;}
 TransferToken reserve(std::uint8_t endpoint,std::uint32_t length,std::uint32_t timeout_ms){
  if(!running_||!next_||!length||length>usb_transport_capacity||!timeout_ms||timeout_ms>2000)return {};
  bool known=false;for(auto e:service_endpoints)known|=e==endpoint;if(!known)return {};
  for(unsigned i=0;i<slots_.size();++i)if(slots_[i].phase==TransferPhase::free){
   const auto ticket=next_;next_=ticket==std::numeric_limits<std::uint64_t>::max()?0:ticket+1;
   slots_[i]={TransferPhase::pending,TransferKind::ok,ticket,endpoint,length,0,0};return {epoch_,ticket,i};
  }
  return {};
 }
 bool complete(TransferToken token,std::int32_t status,std::uint32_t actual){
  if(!running_||token.epoch!=epoch_||token.slot>=slots_.size())return false;
  auto& s=slots_[token.slot];if(s.phase!=TransferPhase::pending||s.ticket!=token.ticket)return false;
  s.phase=TransferPhase::complete;s.status=status;s.actual=0;
  if(status)s.kind=TransferKind::usb_error;
  else if(actual>s.requested)s.kind=TransferKind::overrun;
  else if(!(s.endpoint&0x80)&&actual!=s.requested)s.kind=TransferKind::short_write;
  else{s.kind=TransferKind::ok;s.actual=actual;}
  return true;
 }
 bool begin_cancel(std::uint64_t ticket){
  const auto i=index(ticket);if(i==slots_.size()||slots_[i].phase==TransferPhase::cancelling)return false;
  slots_[i].phase=TransferPhase::cancelling;return true;
 }
 bool cancel_confirmed(TransferToken token){
  if(token.epoch!=epoch_||token.slot>=slots_.size())return false;
  auto& s=slots_[token.slot];if(s.ticket!=token.ticket||s.phase!=TransferPhase::cancelling)return false;
  s={};return true;
 }
 void stop(){running_=false;for(auto& s:slots_)if(s.phase!=TransferPhase::free)s.phase=TransferPhase::cancelling;}
};
}
