#pragma once
#include "archer/channel_bus.hpp"
namespace archer {
struct IfsClmReport {
 unsigned done,total,tx,edcca,cck_cca,ofdm_cca,cck_fa,ofdm_fa;
};
// Linux phy.c CCX init/set/trigger, phy_be.c be_v1 map. Fixed2000ms,
// period62500,unit3=32us; IFS thresholds64/256/1024/4096us.
inline void with_ifs_clm_window(RegisterBus& bus,const std::function<void()>& body){
 if(channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("IFS requires paused scheduler");
 const std::array<unsigned,6> addresses{0x20c00,0x20c28,0x20c2c,0x20c30,0x20c34,0x20c38};
 const std::array<unsigned,6> masks{0xf7,0xfffff000,0xffffffff,0xffffffff,0xffffffff,0xffffffff};
 std::array<unsigned,6> original{};
 for(unsigned i=0;i<6;++i)original[i]=channel_bus_read(bus,addresses[i]);
 if((original[0]&1)&&(original[1]&0x1000)&&!(channel_bus_read(bus,0x20eec)&0x10000))
  throw std::runtime_error("Existing IFS monitor may be ongoing");
 std::exception_ptr failure;
 try{
  channel_bus_write(bus,0x20c00,0xf7,7); // CCX enabled, trigger option1, EDCCA BW20_0.
  channel_bus_write(bus,0x20c28,0xffffc000,(62500u<<16)|(3u<<14));
  for(unsigned i=0,high=2,low=0;i<4;++i){
   channel_bus_write(bus,addresses[2+i],0xffffffff,(high<<16)|0x8000|low);
   low=high+1;high*=4;
  }
  channel_bus_write(bus,0x20c28,0x1000,0x1000);
  body();
 }catch(...){failure=std::current_exception();}
 std::string errors;
 const auto restore=[&](unsigned a,unsigned mask,unsigned value){
  try{channel_bus_write(bus,a,mask,value&mask);}catch(const std::exception& e){errors+=std::string(e.what())+"; ";}
 };
 restore(0x20c00,5,0); // Stop monitoring before restoring its configuration.
 for(unsigned i=2;i<6;++i)restore(addresses[i],masks[i],original[i]);
 restore(addresses[1],masks[1],original[1]);restore(addresses[0],masks[0],original[0]);
 if(!errors.empty())throw std::runtime_error("IFS control restoration failed: "+errors);
 if(failure)std::rethrow_exception(failure);
}
inline void trigger_ifs_clm(RegisterBus& bus){
 channel_bus_write(bus,0x20c28,0x2000,0);
 channel_bus_write(bus,0x20c00,4,0);
 channel_bus_write(bus,0x20c28,0x2000,0x2000);
 channel_bus_write(bus,0x20c00,4,4);
}
inline IfsClmReport read_ifs_clm(RegisterBus& bus){
 const auto total=channel_bus_read(bus,0x20eec),tx=channel_bus_read(bus,0x20ecc),
            cca=channel_bus_read(bus,0x20ed0),fa=channel_bus_read(bus,0x20ed4);
 if(!(total&0x10000))throw std::runtime_error("IFS report not done");
 return {1,total&65535,tx&65535,tx>>16,cca&65535,cca>>16,fa&65535,fa>>16};
}
}
