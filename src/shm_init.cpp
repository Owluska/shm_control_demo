#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "shared_data.hpp"

int main() {
  const std::size_t shm_size = sizeof(SharedMemoryLayout);

  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    std::cerr << "shm_open failed " << std::strerror(errno) << '\n';
  }

  if (ftruncate(fd, shm_size) == -1) {
    std::cerr << "ftruncate failed: " << std::strerror(errno) << '\n';
    close(fd);
    return 1;
  }

  void* ptr =
      mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (ptr == MAP_FAILED) {
    std::cerr << "mmap failed: " << std::strerror(errno) << '\n';
    close(fd);
    return -1;
  }

  auto* shm = static_cast<SharedMemoryLayout*>(ptr);
  // #include <new>
  // this means: Construct a SharedMemoryLayout object exactly at the memory
  // address ptr. auto* shm = new (ptr) SharedMemoryLayout{};

  shm->magic = SHM_MAGIC;
  shm->version = SHM_VERSION;

  shm->target.seq.store(0);
  shm->target.data = ControlTarget{
      .timestamp_ns = 0, .target_speed_mps = 0.0, .target_curvature = 0.0};

  shm->state.seq.store(0);
  shm->state.data =
      VehicleState{.timestamp_ns = 0, .speed_mps = 0.0, .yaw_rate_radps = 0.0};

  shm->command.seq.store(0);
  shm->command.data = ActuatorCommand{
      .timestamp_ns = 0, .throttle = 0.0, .brake = 0.0, .steering = 0.0};

  std::cout << "Shared memory initialized\n";
  std::cout << "Name: " << SHM_NAME << '\n';
  std::cout << "Size: " << shm_size << " bytes\n";

  munmap(ptr, shm_size);
  close(fd);
  return 0;
}