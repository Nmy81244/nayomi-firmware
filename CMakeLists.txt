cmake_minimum_required(VERSION 3.20)
project(nayomi-firmware.elf C CXX ASM)

set(LIB_ROOT ${CMAKE_SOURCE_DIR}/libraries/AT32F402_405_Firmware_Library)

add_executable(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE
        -g3
        -Og
)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        AT32F405CCT7
        USB_OTG_HS
)

target_link_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
        -T${LIB_ROOT}/libraries/cmsis/cm4/device_support/startup/gcc/linker/AT32F405xC_FLASH.ld
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,-Map=output.map
)

target_sources(${PROJECT_NAME} PRIVATE
        core/src/main.cpp
        core/src/nayomi_protocol.cpp
        core/src/nayomi_input.cpp
        core/src/board_pinout.cpp
        core/usb/nayomi_usb.c
        core/usb/nayomi_usb_int.c
        core/usb/composite_cdc_keyboard/cdc_keyboard_class.c
        core/usb/composite_cdc_keyboard/cdc_keyboard_desc.c

        ${LIB_ROOT}/project/at_start_f405/templates/src/at32f402_405_clock.c

        ${LIB_ROOT}/libraries/cmsis/cm4/device_support/system_at32f402_405.c
        ${LIB_ROOT}/libraries/cmsis/cm4/device_support/startup/gcc/startup_at32f402_405.s

        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_acc.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_adc.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_crm.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_flash.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_gpio.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_misc.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_pwc.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_scfg.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_usb.c

        ${LIB_ROOT}/middlewares/usb_drivers/src/usb_core.c
        ${LIB_ROOT}/middlewares/usb_drivers/src/usbd_core.c
        ${LIB_ROOT}/middlewares/usb_drivers/src/usbd_int.c
        ${LIB_ROOT}/middlewares/usb_drivers/src/usbd_sdr.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        core/inc
        core/usb
        core/usb/composite_cdc_keyboard

        ${LIB_ROOT}/libraries/cmsis/cm4/device_support
        ${LIB_ROOT}/libraries/cmsis/cm4/core_support
        ${LIB_ROOT}/libraries/drivers/inc
        ${LIB_ROOT}/project/at_start_f405/templates/inc

        ${LIB_ROOT}/middlewares/usb_drivers/inc
)
