cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor_node_unit)

target_sources(app PRIVATE
  src/test_frame.c
  src/test_alarm.c
  ../../src/frame.c
  ../../src/alarm.c)
target_include_directories(app PRIVATE ../../src)
