cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ble_peripheral_example)

target_sources(app PRIVATE main.c src/my_service.c src/buttons.c src/ht16k33_led.c src/dht22_sensor.c)

target_include_directories(app PRIVATE include)