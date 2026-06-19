cmake_minimum_required(VERSION 3.16)

project(shm_control_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_library(shm_common INTERFACE)

target_include_directories(shm_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(shm_common
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(shm_common
    INTERFACE
        Threads::Threads
        rt
)

add_executable(shm_init src/shm_init.cpp)
target_link_libraries(shm_init PRIVATE shm_common)

add_executable(planner src/planner.cpp)
target_link_libraries(planner PRIVATE shm_common)

add_executable(controller src/controller.cpp)
target_link_libraries(controller PRIVATE shm_common)

add_executable(can_driver_mock src/can_driver_mock.cpp)
target_link_libraries(can_driver_mock PRIVATE shm_common)

add_executable(monitor src/monitor.cpp)
target_link_libraries(monitor PRIVATE shm_common)

# mkdir -p build
# cd build
# cmake ..
# make -j$(nproc)