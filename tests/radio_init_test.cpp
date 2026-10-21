#include "archer/radio_init.hpp"
#include <cassert>
#include <iostream>
#include <map>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> r{{0xfc,0x371},{0x3f0,0x2200000},{0x74,0x901004fd},{0x84,7},{0x842c,0x7ff9},{0x22c24,0},{0x22d24,0},{0x2e004,0}};unsigned writes=0;
 unsigned read(unsigned a,unsigned)override{return r[a];}void write(unsigned a,unsigned,unsigned v)override{++writes;r[a]=v;}void pause()override{}
};
template<class F>void rejects(F f){bool hit=false;try{f();}catch(const std::exception&){hit=true;}assert(hit);}
int main(){archer::RadioTable a{0,{{0x5,0x12345,0},{0x10001,0x54321,0},{0xfe,0,50000}}};auto packets=archer::radio_store_commands(a,7);assert(packets.size()==1);auto& p=packets[0];assert(archer::channel_u32(p.data()+24)==0x07000022&&archer::channel_u32(p.data()+32)==0x154321);
 std::vector<archer::RadioTable> tables{a,{1,{{0x10002,0x123,0}}}};Bus bus;unsigned sent=0,delay=0;
 archer::initialize_radio_tables(bus,tables,{7,8},[&](const auto&){++sent;},[&](unsigned n){delay+=n;},[](unsigned,unsigned){});assert(sent==2&&delay==50000&&bus.r[0x2e004]==0x54321);
 Bus bad;tables[1].entries.push_back({0x100,0,0});rejects([&]{archer::initialize_radio_tables(bad,tables,{7,8},[](const auto&){assert(false);},[](unsigned){},[](unsigned,unsigned){});});assert(bad.writes==0);
 std::cout<<"Radio table/store encoding, ordering and preflight validation OK\n";}
