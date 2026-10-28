#include "archer/passive_channels.hpp"
#include "channel_payload.hpp"
#include <iostream>
int main(int argc,char** argv){
    if(argc!=2||std::string(argv[1])!="--inspect-plans"){
        std::cerr<<"Usage: archer-passive-channel-plan --inspect-plans (offline, no USB)\n";return 2;
    }
    try{
        const archer::Channel1Inputs inputs{true,true,true,true,true};
        for(unsigned channel:{1u,6u,11u}){
            const auto plan=archer::passive_channel_bb_plan(channel,inputs,trusted_channel1_bb);
            std::cout<<"{\"channel\":"<<channel<<",\"frequency_mhz\":"<<archer::passive_channel_frequency(channel)
                     <<",\"executable\":false,\"bb_writes\":[";
            bool comma=false;for(const auto& op:plan){if(comma)std::cout<<',';comma=true;
                std::cout<<"{\"address\":"<<op.address<<",\"mask\":"<<op.mask<<",\"value\":"<<op.value<<"}";}
            std::cout<<"],\"pending\":[\"live MCC history\",\"RFK and restore coordination\",\"power context\",\"capture channel provenance\"]}"<<std::endl;
        }
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
