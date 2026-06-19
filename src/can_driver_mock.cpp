#include "shm_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace {

double clamp(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(value, max_value));
}

struct MockVehicleModel {
    double speed_mps = 0.0;
    double yaw_rate_radps = 0.0;

    void update(const ActuatorCommand& command, double dt_sec) {
        // Very simplified vehicle dynamics:
        //
        // throttle = acceleration
        // brake    = deceleration
        // drag     = speed-dependent resistance

        constexpr double max_accel_mps2 = 2.0;
        constexpr double max_brake_mps2 = 4.0;
        constexpr double drag_coeff = 0.15;

        const double throttle = clamp(command.throttle, 0.0, 1.0);
        const double brake = clamp(command.brake, 0.0, 1.0);

        const double accel_from_throttle = throttle * max_accel_mps2;
        const double decel_from_brake = brake * max_brake_mps2;
        const double drag = drag_coeff * speed_mps;

        const double acceleration =
            accel_from_throttle - decel_from_brake - drag;

        speed_mps += acceleration * dt_sec;
        speed_mps = clamp(speed_mps, 0.0, 20.0);

        // For now steering is just converted to a fake yaw rate.
        yaw_rate_radps = clamp(command.steering, -1.0, 1.0) * speed_mps * 0.1;
    }

    VehicleState makeState() const {
        VehicleState state{};
        state.timestamp_ns = nowNs();
        state.speed_mps = speed_mps;
        state.yaw_rate_radps = yaw_rate_radps;
        return state;
    }
};

}  // namespace

int main() {
    try {
        auto mapping = openExistingSharedMemory();
        auto* shm = mapping.layout();

        std::cout << "CAN driver mock started\n";

        MockVehicleModel model;

        auto last_time = std::chrono::steady_clock::now();

        while (true) {
            const auto now = std::chrono::steady_clock::now();
            const double dt_sec =
                std::chrono::duration<double>(now - last_time).count();
            last_time = now;

            const auto command = readSlot(shm->command);

            model.update(command.data, dt_sec);

            const VehicleState state = model.makeState();
            writeSlot(shm->state, state);

            std::cout
                << "speed=" << state.speed_mps
                << " yaw_rate=" << state.yaw_rate_radps
                << " throttle=" << command.data.throttle
                << " brake=" << command.data.brake
                << '\n';

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}