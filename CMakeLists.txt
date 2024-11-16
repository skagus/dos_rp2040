cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(pico_bl)
pico_sdk_init()
add_executable(pico_bl)
target_sources(pico_bl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/cli.c
    ${CMAKE_CURRENT_LIST_DIR}/sched.c
    ${CMAKE_CURRENT_LIST_DIR}/lld_cdc.c
	${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)

set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(pico_bl PUBLIC
${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(pico_bl PUBLIC pico_stdlib tinyusb_device tinyusb_board hardware_flash hardware_sync)

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(pico_bl PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(pico_bl)

