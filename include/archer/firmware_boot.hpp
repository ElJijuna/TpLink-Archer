#pragma once
#include "archer/register_mailbox.hpp"
#include "archer/firmware_channel.hpp"
#include <functional>
#include <string>
namespace archer {
struct BootImage {std::vector<std::vector<std::uint8_t>> frames;};
// One attempt only. After CPU reset the old firmware cannot be restored by register rollback.
// Callers must hold exclusive USB/project ownership and bind trusted images before entry.
inline void boot_firmware(RegisterBus& bus,const std::vector<BootImage>& images,
    const std::function<void(const std::vector<std::uint8_t>&)>& send,
    const std::function<void(const char*)>& phase,bool usb_without_dcpu=false) {
    if(images.size()!=2)throw std::runtime_error("Expected main and BBMCU0 images");
    for(const auto& image:images) {
        if(image.frames.size()<2)throw std::runtime_error("Incomplete boot image");
        for(unsigned i=0;i<image.frames.size();++i){
            const auto& f=image.frames[i];
            if(f.size()<25 || (channel_u32(f.data())&0x3fff)!=f.size()-24 ||
               ((channel_u32(f.data())>>24)&63)!=(i?14u:13u))
                throw std::runtime_error("Invalid firmware frame");
        }
    }
    auto get=[&](unsigned a){auto v=bus.read(a,4);if(v==0xeaeaeaea||v==0xdeadbeef)throw std::runtime_error("Boot register inaccessible");return v;};
    auto set=[&](unsigned a,unsigned mask,unsigned bits){bus.write(a,4,(get(a)&~mask)|(bits&mask));};
    auto poll=[&](unsigned mask,unsigned bits,const char* message){
        for(unsigned n=0;n<100;++n){
            auto v=get(0x1e0);const auto st=(v>>26)&15;
            if(st==4||st==5||st==6||st==7)throw std::runtime_error("ROM rejected firmware (checksum/security/CV)");
            if((v&mask)==bits)return;
            bus.pause();
        }
        throw std::runtime_error(message);
    };
    phase("preflight");
    if((get(0xfc)&255)!=0x71||((get(0xf0)>>12)&15)!=1||((get(0x3f0)>>16)&0x3ff)!=0x220||
       (get(0x74)&0x90000000)!=0x90000000||(get(0x84)&0x33)||get(0x842c)!=0||
       (get(0x7880)&3)!=3||(get(0xb000)&0x7b0)!=0x430||(get(0xb010)&0x7fff)||
       !(get(0x8420)&0x1000)||(get(0x8d00)&3)!=3||(get(0x9100)&3)!=3||!(get(0x904c)&0xfff))
        throw std::runtime_error("Cold-start prerequisites failed; no reset");
    // Probe every RMW target while the original CPU is still running.
    // A sentinel here must never be discovered only after CPU reset.
    if(!usb_without_dcpu)(void)get(0x888);
    for(auto a:{0x88u,0x188u,0x170u,0x40u,8u,0xd0u}) (void)get(a);
    phase("bbmcu_preinit");
    set(0x842c,0xffff,0x7ff9);
    set(0x84,2,0);set(0x84,1,0);set(0x84,2,2);set(0x84,4,0);
    set(0xd0,1u<<17,0);bus.pause();
    const std::pair<unsigned,unsigned> mcu[]={
        {0x36990,0},{0x36994,0},{0x36998,0},{0x36820,0xfffffffe},
        {0x36800,0xc0000ffe},{0x36808,0x76543210},{0x36814,0xbfbfb000},
        {0x36818,0x0478c009},{0x36800,0xc0000fff},{0x36820,0xffffffff}};
    for(auto [a,v]:mcu)bus.write(a,4,v);
    set(0x188,0xffff,0);
    phase("cpu_reset");
    set(0x88,2,0);set(0x88,1u<<11,1u<<11);set(0x88,2,2);
    bus.write(0x1e0,4,get(0x1e0)&0xc0000000);
    if(!usb_without_dcpu)set(0x888,1,1);
    else phase("experimental_usb_no_dcpu_write");
    bus.write(0x1f0,4,0);bus.write(0x16c,4,0);bus.write(0x1f8,4,0);
    set(0x1e0,0xa00,0xa00); // WLAN + BBMCU0 download-enable, not BBMCU1.
    for(auto a:{0x1f4u,0x1f8u,0x78f0u,0x168u,0x16cu,0x160u,0x164u})bus.write(a,4,0);
    bus.write(0x1a4,4,1u<<21); // W1C interrupt, never read-modify-write.
    set(8,1u<<14,1u<<14);set(0x170,0x600,0);set(0x1e0,0x20100,0);
    bus.write(0x184,4,0);set(0x40,1u<<19,0);
    set(0x1e0,0xf0,0x20); // physical CV=1 -> ROM cut encoding 2.
    auto reason=bus.read(0x1e6,2);bus.write(0x1e6,2,reason&~7u);
    set(0x88,2,0);set(0x88,1u<<11,0);set(0x88,2,2);
    for(unsigned i=0;i<2;++i){
        phase(i?"bbmcu_download":"main_download");
        if(!i)bus.write(0x184,4,0x20248000);
        poll(2,2,"H2C boot path timeout");
        send(images[i].frames.front());
        poll(1,1,"Firmware download path timeout");
        bus.write(0x160,4,0);bus.write(0x164,4,0);
        for(unsigned j=1;j<images[i].frames.size();++j)send(images[i].frames[j]);
        poll(i?0x800:0x200,0,"Image download completion timeout");
    }
    phase("firmware_ready");bus.pause();
    poll(0x3c000000,3u<<26,"Firmware initialization timeout");
}
}
