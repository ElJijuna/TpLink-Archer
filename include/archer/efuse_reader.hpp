#pragma once
#include "archer/register_mailbox.hpp"
#include <chrono>
#include <vector>
namespace archer {
enum class EfuseProfile { board, firmware_security };
struct EfuseMaps { std::vector<std::uint8_t> physical, phycap, security; };
inline EfuseMaps read_ddv_efuse(RegisterBus& bus, EfuseProfile profile=EfuseProfile::board) {
    if(profile!=EfuseProfile::board && profile!=EfuseProfile::firmware_security)
        throw std::runtime_error("Unsupported efuse read profile");
    if ((bus.read(0xfc,4)&255) != 0x71 || ((bus.read(0xf0,4)>>12)&15) != 1 ||
        ((bus.read(0x3f0,4)>>16)&0x3ff) != 0x220)
        throw std::runtime_error("Unsupported chip revision or inactive power state");
    const auto iso = bus.read(0,2), debug = bus.read(0xcc,1), burst = bus.read(0xa4,4);
    const auto power = bus.read(0x68,4), control = bus.read(0x30,4);
    if ((iso & 0xc100) != 0x100 || (burst & (1u<<19)) || (control & 0xc0000000) ||
        burst == 0xeaeaeaea || power == 0xeaeaeaea || control == 0xeaeaeaea)
        throw std::runtime_error("Efuse access controls are not in the verified idle state");
    auto update = [&](unsigned address, unsigned width, std::uint32_t mask, std::uint32_t value) {
        const auto current = bus.read(address,width);
        if (current == 0xeaeaeaea || current == 0xdeadbeef)
            throw std::runtime_error("Efuse control inaccessible");
        bus.write(address,width,(current & ~mask) | (value & mask));
    };
    EfuseMaps maps;
    std::exception_ptr failure;
    try {
        update(0x68,4,1u<<16,0);
        if (((bus.read(0x3f0,4)>>16)&0x3ff) != 0x220)
            throw std::runtime_error("Efuse access state did not remain active");
        update(0xcc,1,4,4);
        update(0,2,1u<<14,1u<<14); bus.pause();
        update(0,2,1u<<15,1u<<15);
        update(0,2,1u<<8,0);
        update(0xa4,4,1u<<19,1u<<19);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        auto dump = [&](unsigned start, unsigned size) {
            std::vector<std::uint8_t> bytes(size);
            for (unsigned offset=0; offset<size; offset+=4) {
                if (std::chrono::steady_clock::now() >= deadline)
                    throw std::runtime_error("Efuse acquisition deadline exceeded");
                // Read mode only: no programming mode or payload write is exposed.
                bus.write(0x30,4,start+offset);
                bool ready=false;
                for (unsigned attempt=0;attempt<20;++attempt) {
                    const auto status=bus.read(0x30,4);
                    if (status == 0xeaeaeaea || status == 0xdeadbeef)
                        throw std::runtime_error("Efuse read control inaccessible");
                    if (status & (1u<<29)) { ready=true; break; }
                    bus.pause();
                }
                if (!ready) throw std::runtime_error("Efuse read timeout; no retry");
                const auto value=bus.read(0x34,4);
                for(unsigned i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
            }
            return bytes;
        };
        if(profile==EfuseProfile::firmware_security)maps.security=dump(0x1580,4);
        else {maps.physical=dump(0,0x1300);maps.phycap=dump(0x1700,0x38);}
    } catch (...) { failure=std::current_exception(); }
    // Try every cleanup step even when a preceding restoration transfer fails.
    std::string restoration;
    auto restore = [&](unsigned address,unsigned width,std::uint32_t mask,std::uint32_t value) {
        try {
            update(address,width,mask,value);
            if ((bus.read(address,width)&mask)!=(value&mask))
                throw std::runtime_error("Efuse control restoration mismatch");
        } catch(const std::exception& error) { restoration += std::string(error.what()) + "; "; }
    };
    restore(0xa4,4,1u<<19,burst);
    restore(0,2,1u<<8,1u<<8);
    restore(0,2,1u<<15,iso); bus.pause();
    restore(0,2,1u<<14,iso);
    restore(0,2,1u<<8,iso);
    restore(0xcc,1,4,debug);
    restore(0x68,4,1u<<16,power);
    if (!restoration.empty()) throw std::runtime_error("Efuse restoration failed: " + restoration);
    if(failure) std::rethrow_exception(failure);
    return maps;
}
}
