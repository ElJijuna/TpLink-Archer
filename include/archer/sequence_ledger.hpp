#pragma once
#include <array>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
namespace archer {
// Durable, non-wrapping sequence ownership for one physical adapter epoch.
// Reserve before USB transmission; failures/timeouts never return a sequence.
class SequenceLedger {
    int fd_ = -1;
public:
    explicit SequenceLedger(const std::string& path) {
        fd_ = open(path.c_str(), O_RDWR | O_CREAT | O_NOFOLLOW, 0600);
        if (fd_ < 0) throw std::runtime_error(std::string("Cannot open sequence ledger: ")+std::strerror(errno));
        struct stat info{};
        if (fstat(fd_, &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1) {
            close(fd_); fd_ = -1;
            throw std::runtime_error("Sequence ledger must be a single regular file");
        }
        if (flock(fd_, LOCK_EX | LOCK_NB) != 0) {
            close(fd_); fd_ = -1;
            throw std::runtime_error("Another session owns the sequence ledger");
        }
    }
    ~SequenceLedger() { if (fd_ >= 0) close(fd_); }
    SequenceLedger(const SequenceLedger&) = delete;
    SequenceLedger& operator=(const SequenceLedger&) = delete;
    unsigned reserve() {
        std::array<unsigned char, 264> state{};
        constexpr std::array<unsigned char,8> magic{'A','R','C','H','S','E','Q',1};
        const auto count = pread(fd_, state.data(), state.size(), 0);
        if (count == 0) {
            std::copy(magic.begin(), magic.end(), state.begin());
            state[8] = state[9] = 1; // Previously used physical test sequences.
        } else {
            if (count != static_cast<ssize_t>(state.size()) ||
                !std::equal(magic.begin(), magic.end(), state.begin()) ||
                lseek(fd_, 0, SEEK_END) != static_cast<off_t>(state.size()))
                throw std::runtime_error("Corrupt sequence ledger; refusing sequence reuse");
        }
        for (unsigned i = 0; i < 256; ++i) {
            if (state[8+i] > 1) throw std::runtime_error("Invalid sequence reservation");
        }
        unsigned sequence = 0;
        while (sequence < 256 && state[8+sequence]) ++sequence;
        if (sequence == 256) throw std::runtime_error("Sequence epoch exhausted; no wrap or automatic reset");
        state[8+sequence] = 1;
        if (pwrite(fd_, state.data(), state.size(), 0) != static_cast<ssize_t>(state.size()) || fsync(fd_) != 0)
            throw std::runtime_error("Cannot persist sequence reservation; no command allowed");
        return sequence;
    }
};
}
