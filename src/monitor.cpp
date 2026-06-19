#include "shm_utils.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    try {
        auto mapping = openExistingSharedMemory();
        auto* shm = mapping.layout();

        std::cout << "Monitor started\n";

        while (true) {
            const auto target = readSlot(shm->target);
            const auto state = readSlot(shm->state);
            const auto command = readSlot(shm->command);

            std::cout
            << "target_seq=" << target.seq
            << " target_age_ms=" << ageMs(target.data.timestamp_ns)
            << " target_speed=" << target.data.target_speed_mps
            << " state_seq=" << state.seq
            << " state_age_ms=" << ageMs(state.data.timestamp_ns)
            << " state_speed=" << state.data.speed_mps
            << " command_seq=" << command.seq
            << " command_age_ms=" << ageMs(command.data.timestamp_ns)
            << " throttle=" << command.data.throttle
            << " brake=" << command.data.brake
            << " steering=" << command.data.steering
            << '\n';

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}