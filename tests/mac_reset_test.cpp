#include "archer/mac_reset.hpp"
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> r{{0xfc,0x71},{0xf0,0x1000},{0x1e0,0xc000022},{0x84,7},{0x842c,0x7ff97ff9},{0x88,0x567},{0x7880,7},{0xb000,0x80436},{0xb010,0x80000000},{0x8420,0x1001000},{0x8400,0xffff4e80},{0x8404,0xffffffff},{0x8d00,3},{0x9100,3}};
 std::vector<unsigned> writes;
 unsigned read(unsigned a,unsigned)override{return r[a];}
 void write(unsigned a,unsigned,unsigned v)override{if(a==0x8c08){assert((r[0x88]&0x802)==0x800);assert(!(r[0x84]&7));assert(!(r[0x7880]&3));}writes.push_back(a);r[a]=v;}
 void pause()override{}
};
int main(){Bus b;archer::reset_to_usb2_scc(b,[](unsigned){});assert(((b.r[0x9008]>>16)&0x1fff)==2059);assert((b.r[0x904c]&0xfff)==24);assert(!(b.r[0x1e0]>>26)&&!(b.r[0x88]&2));assert((b.r[0xb000]&0x7b0)==0x430&&(b.r[0x7880]&3)==3);
 Bus invalid;invalid.r[0x1e0]=0xc000000;bool caught=false;try{archer::reset_to_usb2_scc(invalid,[](unsigned){});}catch(const std::exception&){caught=true;}assert(caught&&invalid.writes.empty());std::cout<<"MAC reset precedes SCC repartition, wrong lifecycle rejected OK\n";}
