cmake_minimum_required(VERSION 3.20)
project(nayomi-firmware.elf C CXX ASM)     # ASM must be enabled explicitly or the .s startup file won't compile

set(LIB_ROOT ${CMAKE_SOURCE_DIR}/libraries/AT32F402_405_Firmware_Library)

add_executable(${PROJECT_NAME})

target_link_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
        -T${LIB_ROOT}/libraries/cmsis/cm4/device_support/startup/gcc/linker/AT32F405xC_FLASH.ld
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,-Map=output.map
)

target_compile_definitions(${PROJECT_NAME} PRIVATE AT32F405CCT7)

target_sources(${PROJECT_NAME} PRIVATE
        src/main.cpp
        ${LIB_ROOT}/libraries/cmsis/cm4/device_support/system_at32f402_405.c
        ${LIB_ROOT}/libraries/cmsis/cm4/device_support/startup/gcc/startup_at32f402_405.s
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_crm.c
        ${LIB_ROOT}/libraries/drivers/src/at32f402_405_gpio.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        src                                                         # for your at32f402_405_conf.h — see note below
        ${LIB_ROOT}/libraries/cmsis/cm4/device_support
        ${LIB_ROOT}/libraries/cmsis/cm4/core_support
        ${LIB_ROOT}/libraries/drivers/inc
)