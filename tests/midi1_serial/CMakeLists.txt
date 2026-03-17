# Zephyr MIDI1 Driver Loopback Test
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Set the bindings path so devicetree can find our custom bindings
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(midi1_driver_test)

# We include the driver sources here to simplify for the test app
target_sources(app PRIVATE 
    src/main.c
    ../../drivers/midi1_serial.c
    ../../drivers/midi1.c
)

# Include the driver directory so we can find the headers
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../drivers)
