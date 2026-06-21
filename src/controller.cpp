#include <chrono>
#include <iostream>
#include <thread>

#include "shm_utils.hpp"

double clamp(double value, double min_value, double max_value) {
  if (value < min_value) {
    return min_value;
  }

  if (value > max_value) {
    return max_value;
  }

  return value;
}

ActuatorCommand getActuatorCommand(const VehicleState& state,
                                   const ControlTarget& target) {
  constexpr double kp_speed = 0.4;

  const double speed_error = target.target_speed_mps - state.speed_mps;

  ActuatorCommand command{};
  command.timestamp_ns = nowNs();

  if (speed_error >= 0.0) {
    command.throttle = clamp(kp_speed * speed_error, 0.0, 1.0);
    command.brake = 0.0;
  } else {
    command.throttle = 0.0;
    command.brake = clamp(-kp_speed * speed_error, 0.0, 1.0);
  }

  command.steering = clamp(target.target_curvature, -1.0, 1.0);

  return command;
}

ActuatorCommand safeStop() {
  return ActuatorCommand{
      .timestamp_ns = nowNs(), .throttle = 0.0, .brake = 1.0, .steering = 0.0};
}

int main() {
  try {
    auto mapping = openExistingSharedMemory();
    auto* shm = mapping.layout();

    std::cout << "Controller started\n";

    while (true) {
      const auto state = readSlot(shm->state);
      const auto target = readSlot(shm->target);

      ActuatorCommand command{};

      constexpr double max_input_age_ms = 200.0;

      const double target_age_ms = ageMs(target.data.timestamp_ns);
      const double state_age_ms = ageMs(state.data.timestamp_ns);

      if (!isFresh(target.data.timestamp_ns, max_input_age_ms)) {
        command = safeStop();
      } else if (!isFresh(state.data.timestamp_ns, max_input_age_ms)) {
        command = safeStop();
      } else {
        command = getActuatorCommand(state.data, target.data);
      }

      writeSlot(shm->command, command);

      std::cout << "target_speed=" << target.data.target_speed_mps
                << " target_age_ms=" << target_age_ms
                << " state_speed=" << state.data.speed_mps
                << " state_age_ms=" << state_age_ms
                << " throttle=" << command.throttle
                << " brake=" << command.brake
                << " steering=" << command.steering << '\n';

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}