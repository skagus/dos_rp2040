cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(my_boot)
pico_sdk_init()
add_executable(my_boot)
target_sources(my_boot PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
	${CMAKE_CURRENT_LIST_DIR}/src/cli.c
	${CMAKE_CURRENT_LIST_DIR}/src/fw_util.c
	${CMAKE_CURRENT_LIST_DIR}/src/lfs.c
	${CMAKE_CURRENT_LIST_DIR}/src/lfs_util.c
	${CMAKE_CURRENT_LIST_DIR}/src/lfs_port.c
	${CMAKE_CURRENT_LIST_DIR}/src/dos.c
)

#	${CMAKE_CURRENT_LIST_DIR}/src/lld_cdc.c
#	${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c


set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(my_boot PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/src
)

pico_enable_stdio_usb(my_boot 0)
pico_enable_stdio_uart(my_boot 1)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(my_boot PUBLIC
	pico_stdlib
	hardware_flash
	hardware_sync
	hardware_uart
	"-Wl,--wrap=malloc -Wl,--wrap=free"
)
#	tinyusb_device
#	tinyusb_board


# Linker script 변경.
#pico_set_linker_script(my_boot ${CMAKE_CURRENT_LIST_DIR}/private_map.ld)
# below line works with warning (re-declartion of FLASH/RAM)
target_link_options(my_boot PRIVATE "-T${CMAKE_CURRENT_LIST_DIR}/private_map.ld")

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(my_boot PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(my_boot)

