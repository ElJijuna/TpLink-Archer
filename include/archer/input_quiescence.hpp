#pragma once
#include <chrono>
namespace archer {
// receive(index, timeout_ms) must support a nonblocking poll at timeout0.
// Do not wait on an idle endpoint while the other has completed data queued.
template<class Receive, class Now>
bool drain_input_until_quiet(Receive receive, Now now) {
    const auto deadline=now()+std::chrono::seconds(3);
    while(now()<deadline) {
        const bool a=receive(0,0),b=receive(1,0);
        if(a||b)continue;
        const bool waited_a=receive(0,10),waited_b=receive(1,10);
        if(!waited_a&&!waited_b)return true;
    }
    return false;
}
}
