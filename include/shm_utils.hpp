#pragma once

#include "shared_data.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

inline uint64_t nowNs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

inline double ageMs(uint64_t timestamp_ns) {
    if (timestamp_ns == 0) {
        return -1.0;
    }
    const uint64_t now = nowNs();
    return static_cast<double>(now - timestamp_ns) / 1'000'000.0;
}

bool isFresh(uint64_t timestamp_ns, double max_age_ms) {
    const double age_ms = ageMs(timestamp_ns);
    return age_ms >= 0.0 && age_ms <= max_age_ms;
}

template <typename T>
inline void writeSlot(SeqlockSlot<T>& slot, const T& data) {
    slot.seq.fetch_add(1, std::memory_order_release);  // odd: write started
    slot.data = data;
    slot.seq.fetch_add(1, std::memory_order_release);  // even: write finished
}

template <typename T>
struct SlotSnapshot {
    T data;
    uint64_t seq;
};

template <typename T>
inline SlotSnapshot<T> readSlot(const SeqlockSlot<T>& slot) {
    T copy{};

    while (true) {
        const uint64_t seq_before = slot.seq.load(std::memory_order_acquire);

        if (seq_before % 2 != 0) {
            continue;
        }

        copy = slot.data;

        const uint64_t seq_after = slot.seq.load(std::memory_order_acquire);

        if (seq_before == seq_after) {
            return SlotSnapshot<T>{
                .data = copy,
                .seq = seq_after
            };
        }
    }
}

inline bool checkSharedMemoryLayout(const SharedMemoryLayout* shm) {
    return shm != nullptr &&
           shm->magic == SHM_MAGIC &&
           shm->version == SHM_VERSION;
}

struct SharedMemoryMapping {
    int fd = -1;
    void* ptr = MAP_FAILED;
    std::size_t size = sizeof(SharedMemoryLayout);

    SharedMemoryLayout* layout() {
        return static_cast<SharedMemoryLayout*>(ptr);
    }

    const SharedMemoryLayout* layout() const {
        return static_cast<const SharedMemoryLayout*>(ptr);
    }

    void closeMapping() {
        if (ptr != MAP_FAILED) {
            munmap(ptr, size);
            ptr = MAP_FAILED;
        }

        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }

    ~SharedMemoryMapping() {
        closeMapping();
    }

    SharedMemoryMapping() = default;

    SharedMemoryMapping(const SharedMemoryMapping&) = delete;
    SharedMemoryMapping& operator=(const SharedMemoryMapping&) = delete;

    SharedMemoryMapping(SharedMemoryMapping&& other) noexcept {
        fd = other.fd;
        ptr = other.ptr;
        size = other.size;

        other.fd = -1;
        other.ptr = MAP_FAILED;
    }

    SharedMemoryMapping& operator=(SharedMemoryMapping&& other) noexcept {
        if (this != &other) {
            closeMapping();

            fd = other.fd;
            ptr = other.ptr;
            size = other.size;

            other.fd = -1;
            other.ptr = MAP_FAILED;
        }

        return *this;
    }
};

inline SharedMemoryMapping openExistingSharedMemory() {
    SharedMemoryMapping mapping;

    mapping.fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (mapping.fd == -1) {
        throw std::runtime_error(
            std::string("shm_open failed: ") + std::strerror(errno) +
            ". Did you run ./shm_init first?"
        );
    }

    mapping.ptr = mmap(
        nullptr,
        mapping.size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mapping.fd,
        0
    );

    if (mapping.ptr == MAP_FAILED) {
        const std::string error = std::string("mmap failed: ") + std::strerror(errno);
        mapping.closeMapping();
        throw std::runtime_error(error);
    }

    if (!checkSharedMemoryLayout(mapping.layout())) {
        mapping.closeMapping();
        throw std::runtime_error("Shared memory has wrong magic/version");
    }

    return mapping;
}