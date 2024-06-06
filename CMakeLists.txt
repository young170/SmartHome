#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_lbs)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/bluetooth.c
  src/buttons.c
  src/led.c
  src/sensors.c
  main.c
)

target_include_directories(app PRIVATE include)

# NORDIC SDK APP END
