# Robotic Dog - legs firmware (Teensy 4.1, Zephyr RTOS, C++20)
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(robotic_dog_legs)

# Which leg this build is for: 0=FR 1=FL 2=RR 3=RL (Conventions.md).
# scripts/02_build sets LEG_ID and the matching IP for every leg from
# scripts/legs.csv; include/config.hpp fails the build if the two disagree.
set(LEG_ID 0 CACHE STRING "Leg index 0..3 (FR FL RR RL)")

target_sources(app PRIVATE
    src/main.cpp
    src/shared.cpp
    src/hal.cpp
    src/rx_thread.cpp
    src/control_thread.cpp
    src/tx_thread.cpp
    src/supervisor_thread.cpp
)
target_include_directories(app PRIVATE include)
target_compile_definitions(app PRIVATE LEG_ID=${LEG_ID})
