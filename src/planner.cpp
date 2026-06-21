#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include "shm_utils.hpp"

ControlTarget getControlTarget(double t_sec) {
  ControlTarget target{};
  target.timestamp_ns = nowNs();
  target.target_speed_mps = 3.0 + 2.0 * std::sin(t_sec);
  target.target_curvature = 0.0;
  return target;
}

int main() {
  try {
    auto mapping = openExistingSharedMemory();
    auto* shm = mapping.layout();

    std::cout << "Planner started\n";

    const auto start = std::chrono::steady_clock::now();

    while (true) {
      const auto now = std::chrono::steady_clock::now();
      const double t_sec = std::chrono::duration<double>(now - start).count();

      const ControlTarget target = getControlTarget(t_sec);
      writeSlot(shm->target, target);

      std::cout << "target_speed = " << target.target_speed_mps << " m/s\n";

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}