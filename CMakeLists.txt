# Robotic Dog - legs firmware (Teensy 4.1, Zephyr RTOS, C++20)
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(robotic_dog_legs)

target_sources(app PRIVATE
    src/main.cpp
)
