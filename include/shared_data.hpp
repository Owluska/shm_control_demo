#pragma once

#include <atomic>
#include <cstdint>

struct ControlTarget {
    uint64_t timestamp_ns;
    double target_speed_mps;
    double target_curvature;
};

struct VehicleState {
    uint64_t timestamp_ns;
    double speed_mps;
    double yaw_rate_radps;
};

struct ActuatorCommand {
    uint64_t timestamp_ns;
    double throttle;
    double brake;
    double steering;
};

template <typename T>
struct SeqlockSlot {
    std::atomic<uint64_t> seq;
    T data;
};

struct SharedMemoryLayout {
    uint32_t magic;
    uint32_t version;

    SeqlockSlot<ControlTarget> target;
    SeqlockSlot<VehicleState> state;
    SeqlockSlot<ActuatorCommand> command;
};

inline constexpr const char* SHM_NAME = "/shm_control_demo";
inline constexpr uint32_t SHM_MAGIC = 0xC0117A01;
inline constexpr uint32_t SHM_VERSION = 1;