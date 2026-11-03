#include "archer/input_quiescence.hpp"
#include <cassert>
#include <iostream>
struct Replay {
    // Tag7 logged879 successful callbacks, but consumed only242 transfers.
    unsigned queued=879,consumed=0,empty_waits=0;
    std::chrono::steady_clock::time_point time{};
    bool receive(unsigned index,unsigned timeout) {
        if(index==0&&queued){--queued;++consumed;time+=std::chrono::milliseconds(1);return true;}
        time+=std::chrono::milliseconds(timeout);if(timeout)++empty_waits;return false;
    }
};
int main(){
    Replay old;
    const auto deadline=old.time+std::chrono::seconds(3);bool quiet=false;
    while(old.time<deadline){const bool a=old.receive(0,10),b=old.receive(1,10);if(!a&&!b){quiet=true;break;}}
    assert(!quiet&&old.queued); // Original pacing fails with a finite backlog.
    Replay fixed;
    assert(archer::drain_input_until_quiet([&](unsigned i,unsigned t){return fixed.receive(i,t);},[&]{return fixed.time;}));
    assert(fixed.consumed==879&&!fixed.queued&&fixed.empty_waits==2);
    Replay other;
    assert(archer::drain_input_until_quiet([&](unsigned i,unsigned t){return other.receive(1-i,t);},[&]{return other.time;}));
    assert(other.consumed==879);
    auto time=std::chrono::steady_clock::time_point{};
    assert(!archer::drain_input_until_quiet([&](unsigned,unsigned){time+=std::chrono::milliseconds(1);return true;},[&]{return time;}));
    std::cout<<"Finite input backlog drains without idle-endpoint delay; continuous input stays bounded\n";
}
