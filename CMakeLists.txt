# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

##################
# Set any variables required for importing libraries
SET(FREERTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS)

message("FreeRTOS Kernel located in ${FREERTOS_PATH}")
##################

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(${FREERTOS_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Matrix_NeoPixel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Matrix_NeoPixel src/main.c src/oled_ctrl.c src/neopin.c src/joystick.c src/exam.c include/ssd1306_i2c.c )

pico_set_program_name(Matrix_NeoPixel "Matrix_NeoPixel")
pico_set_program_version(Matrix_NeoPixel "0.1")

# Generate PIO header
# pico_generate_pio_header(Matrix_NeoPixel ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

pico_generate_pio_header(Matrix_NeoPixel ${CMAKE_CURRENT_LIST_DIR}/include/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Matrix_NeoPixel 0)
pico_enable_stdio_usb(Matrix_NeoPixel 1)

# Add the standard library to the build
target_link_libraries(Matrix_NeoPixel
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Matrix_NeoPixel PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/src
)

# Add any user requested libraries
target_link_libraries(Matrix_NeoPixel 
        hardware_dma
        hardware_pio
        hardware_adc
        hardware_dma
        hardware_timer
        hardware_irq
        pico_time
        pico_rand
        hardware_i2c
        FreeRTOS-Kernel-Heap4
        )

pico_add_extra_outputs(Matrix_NeoPixel)

