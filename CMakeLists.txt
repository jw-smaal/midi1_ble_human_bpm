# @file CMakeLists.txt for the MIDI1.0 BLE heartrate sensor
#
# @author Jan-Willem Smaal <usenet@gispen.org> 
# @date 20260119 
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_hr)

# for the USB MIDI sources
include(common/common.cmake)

# Application logic
FILE(GLOB app_sources src/*.c)

# Hardware and driver logic
FILE(GLOB driver_sources drivers/*.c)

target_include_directories(app PRIVATE
	drivers
)

target_sources(app PRIVATE
	${app_sources}
	${driver_sources})

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
