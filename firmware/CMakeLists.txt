cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(korra)

# Produce compile_commands.json for IDE.
# This is produced in the build directory, e.g. `build-frdm_rw612/compile_commands.json`
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src/credentials)
add_subdirectory(src/cloud)
add_subdirectory(src/events)
add_subdirectory(src/internet)
add_subdirectory(src/sensors)
add_subdirectory(src/time)
add_subdirectory(src/utils)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
