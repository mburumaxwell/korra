cmake_minimum_required(VERSION 3.20.0)

target_include_directories(app PRIVATE .)

file(GLOB EVENTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(app PRIVATE ${EVENTS_SOURCES})
