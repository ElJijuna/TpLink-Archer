#include "archer/sequence_ledger.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
template<class F> static void rejects(F f) {
    bool threw = false; try { f(); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);
}
int main() {
    char pattern[] = "/tmp/archer-ledger-XXXXXX";
    const auto temp = mkdtemp(pattern); assert(temp);
    const auto path = std::string(temp) + "/ledger";
    {
        archer::SequenceLedger ledger(path);
        assert(ledger.reserve() == 2);
        rejects([&] { archer::SequenceLedger competing(path); });
    }
    {
        archer::SequenceLedger ledger(path);
        assert(ledger.reserve() == 3); // Reopen cannot recycle prior reservation.
        for (unsigned i = 4; i < 256; ++i) assert(ledger.reserve() == i);
        rejects([&] { (void)ledger.reserve(); });
    }
    { std::ofstream corrupt(path, std::ios::binary | std::ios::trunc); corrupt << "bad"; }
    {
        archer::SequenceLedger ledger(path);
        rejects([&] { (void)ledger.reserve(); });
    }
    const auto link = std::string(temp) + "/link";
    std::filesystem::create_symlink(path, link);
    rejects([&] { archer::SequenceLedger symlink(link); });
    std::filesystem::remove_all(temp);
    std::cout << "Durable sequence reservations, exclusivity and exhaustion: OK\n";
}
