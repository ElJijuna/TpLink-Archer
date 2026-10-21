#pragma once
#include "archer/rf_transport.hpp"
#include "archer/firmware_channel.hpp"
namespace archer {
struct RadioEntry {unsigned address,value,delay_us;};
struct RadioTable {unsigned path;std::vector<RadioEntry> entries;};
inline std::vector<std::vector<std::uint8_t>> radio_store_commands(const RadioTable& table,unsigned sequence){
    if(table.path>1||sequence>255)throw std::invalid_argument("Invalid radio path/sequence");
    std::vector<unsigned> words;
    for(auto e:table.entries){
        if(e.delay_us){if(e.delay_us!=50000||e.address!=0xfe)throw std::invalid_argument("Unsupported RF delay");continue;}
        (void)be_rf_write(table.path,e.address,e.value);
        if(e.address>=0x100)words.push_back((e.address<<20)|e.value);
    }
    if(words.size()>1500)throw std::invalid_argument("RF store exceeds three pages");
    const auto pages=(words.size()+499)/500;
    if(sequence+pages>256)throw std::invalid_argument("Radio store sequence overflow");
    std::vector<std::vector<std::uint8_t>> result;
    for(unsigned first=0,page=0;first<words.size();first+=500,++page){
        std::vector<std::uint8_t> payload(4*std::min<unsigned>(500,words.size()-first));
        for(unsigned i=0;i<payload.size()/4;++i)channel_put32(payload.data()+4*i,words[first+i]);
        result.push_back(wrap_h2c_usb_be(encode_h2c(2,table.path?9:8,page,sequence+page,payload)));
    }
    return result;
}
// Stateful table initialization. A failed write cannot be rolled back as a table.
// Caller supplies powered BB/RF, exclusive ownership and persistent sequence reservations.
inline void initialize_radio_tables(RegisterBus& bus,const std::vector<RadioTable>& tables,
    const std::vector<unsigned>& sequences,const std::function<void(const std::vector<std::uint8_t>&)>& send,
    const std::function<void(unsigned)>& delay_us,const std::function<void(unsigned,unsigned)>& progress){
    if(tables.size()!=2||sequences.size()!=2||tables[0].path!=0||tables[1].path!=1)
        throw std::invalid_argument("Expected radio tables A/B in order");
    std::vector<std::vector<std::vector<std::uint8_t>>> commands;
    for(unsigned i=0;i<2;++i){
        if(tables[i].entries.empty()||tables[i].entries.size()>4096)throw std::invalid_argument("Invalid radio table length");
        commands.push_back(radio_store_commands(tables[i],sequences[i]));
        // Each path must reserve enough non-overlapping contiguous sequences externally.
        if(commands.back().size()!=1)throw std::invalid_argument("Native radio initializer supports one reserved page per path");
    }
    if(sequences[0]==sequences[1])throw std::invalid_argument("Radio store sequence reused");
    BeRfTransport rf(bus);
    for(unsigned path=0;path<2;++path){
        unsigned count=0;
        for(auto e:tables[path].entries){if(e.delay_us)delay_us(e.delay_us);else rf.submit_full_write(path,e.address,e.value);if(++count%128==0)progress(path,count);}
        for(const auto& command:commands[path])send(command);
        progress(path,count);
    }
}
}
