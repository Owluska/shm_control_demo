#include "shm_utils.hpp"

#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

int main() {
    try {
        auto mapping = openExistingSharedMemory();
        auto* shm = mapping.layout();

        std::cout << "Monitor started\n";

        std::ofstream log_file("shm_log.csv");

        log_file
            << "time_ns,"
            << "target_seq,target_age_ms,target_speed,"
            << "state_seq,state_age_ms,state_speed,"
            << "command_seq,command_age_ms,throttle,brake,steering\n";

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

            const uint64_t t_ns = nowNs();

            log_file
                << t_ns << ','
                << target.seq << ','
                << ageMs(target.data.timestamp_ns) << ','
                << target.data.target_speed_mps << ','
                << state.seq << ','
                << ageMs(state.data.timestamp_ns) << ','
                << state.data.speed_mps << ','
                << command.seq << ','
                << ageMs(command.data.timestamp_ns) << ','
                << command.data.throttle << ','
                << command.data.brake << ','
                << command.data.steering << '\n';

            log_file.flush();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}