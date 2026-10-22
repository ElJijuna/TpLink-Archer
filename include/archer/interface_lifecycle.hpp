#pragma once
#include "archer/interface_cam.hpp"
#include "archer/interface_tables.hpp"
#include "archer/command_tracker.hpp"
#include <set>
namespace archer {
enum class ContextPhase { building, ready, removing, cleanup_acknowledged, uncertain };
enum class ContextAck { receive, done, none };
class InterfaceContextLifecycle {
    std::array<std::vector<std::uint8_t>,9> frames_;
    std::vector<CommandTracker> trackers_;
    std::array<bool,9> issued_{},sent_{};
    unsigned step_=0;
    ContextPhase phase_=ContextPhase::building;
    void advance(){
        ++step_;
        if(step_==6)phase_=ContextPhase::ready;
        if(step_==9)phase_=ContextPhase::cleanup_acknowledged;
    }
public:
    // Caller must reserve indices and nine non-reused ledger sequences before
    // constructing. No I/O, automatic retries, destructor cleanup or TX resume.
    InterfaceContextLifecycle(const UnlinkedCamContext& c,const std::array<unsigned,9>& seq,
        bool resources_exclusive,bool scheduler_paused,bool initial_macid_paused,
        StationCapability capability=StationCapability::legacy,unsigned wmm=0){
        if(!resources_exclusive||!scheduler_paused||c.mac_id>127)
            throw std::invalid_argument("Context requires exclusive resources and paused scheduler");
        if(std::set<unsigned>(seq.begin(),seq.end()).size()!=seq.size())
            throw std::invalid_argument("Context sequences must be distinct");
        frames_={encode_station_macid_pause_sleep(seq[0],c.mac_id,false),
            encode_unlinked_station_role(seq[1],c.mac_id,c.port,StationRoleAction::create),
            encode_unlinked_station_join(seq[2],c.mac_id,c.port,wmm,capability),
            encode_unlinked_cam_v0(seq[3],c,StationRoleAction::create),
            encode_default_station_cmac_g7(seq[4],c.mac_id,false),
            encode_default_station_dmac_v2(seq[5],c.mac_id),
            encode_unlinked_station_role(seq[6],c.mac_id,c.port,StationRoleAction::remove),
            encode_unlinked_cam_v0(seq[7],c,StationRoleAction::remove),
            encode_station_macid_pause_sleep(seq[8],c.mac_id,initial_macid_paused)};
        for(const auto& f:frames_){const auto w=channel_u32(f.data());trackers_.emplace_back(w&3,(w>>2)&63,(w>>8)&255,w>>24);}
    }
    ContextPhase phase()const{return phase_;}
    unsigned step()const{return step_;}
    ContextAck ack_policy()const{
        if(step_>=9)throw std::logic_error("No pending context command");
        return step_==0||step_==8?ContextAck::receive:step_==5?ContextAck::none:ContextAck::done;
    }
    const std::vector<std::uint8_t>& issue(){
        if((phase_!=ContextPhase::building&&phase_!=ContextPhase::removing)||issued_[step_])
            throw std::logic_error("Context command cannot be issued or retried");
        issued_[step_]=true;return frames_[step_];
    }
    void sent_successfully(){
        if(step_>=9||phase_==ContextPhase::uncertain||!issued_[step_]||sent_[step_])
            throw std::logic_error("Unexpected context transport success");
        sent_[step_]=true;
        if(ack_policy()==ContextAck::none)advance(); // Submission only, not firmware acceptance.
    }
    bool observe(const std::uint8_t* data,std::size_t size){
        if(phase_==ContextPhase::uncertain)return false;
        try{
            for(unsigned i=0;i<9;++i){
                if(!issued_[i]||!sent_[i]||!trackers_[i].observe(data,size))continue;
                const auto& t=trackers_[i];
                if(t.completion&&*t.completion!=0){phase_=ContextPhase::uncertain;return true;}
                if(i==step_&&((ack_policy()==ContextAck::receive&&t.received)||
                    (ack_policy()==ContextAck::done&&t.completion==0)))advance();
                return true;
            }
        }catch(...){phase_=ContextPhase::uncertain;throw;}
        return false;
    }
    void begin_removal(){
        if(phase_!=ContextPhase::ready)throw std::logic_error("Context not ready for controlled removal");
        phase_=ContextPhase::removing;
    }
    void mark_transport_uncertain(){
        if(step_>=9||!issued_[step_]||phase_==ContextPhase::ready)
            throw std::logic_error("No outstanding context command");
        phase_=ContextPhase::uncertain;
    }
};
}
