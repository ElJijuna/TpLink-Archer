#include "archer/usb_request.hpp"
#include <cassert>
#include <iostream>
int main() {
    using namespace archer;
    const std::array<std::uint8_t, 8> expected{0xc0,5,0x56,0x34,0x12,0,4,0};
    assert(register_request(Direction::read, 0x123456, 4) == expected);
    auto write = register_request(Direction::write, 0, 1);
    assert(write[0] == 0x40 && write[6] == 1);
    for (auto address : {0x1000000u, 0xffffffu}) {
        bool rejected = false;
        try { (void)register_request(Direction::read, address, 4); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
    for (auto width : {0, 3, 5}) {
        bool rejected = false;
        try { (void)register_request(Direction::read, 0, width); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
    std::cout << "USB request encoding: OK\n";
}
