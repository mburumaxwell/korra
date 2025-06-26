cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(korra)

add_subdirectory(src/credentials)
add_subdirectory(src/cloud)
add_subdirectory(src/events)
add_subdirectory(src/internet)
add_subdirectory(src/sensors)
add_subdirectory(src/time)
add_subdirectory(src/utils)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
