cmake_minimum_required(VERSION 3.20)
project(nokia_snake C)

set(CMAKE_C_STANDARD 11)

add_executable(sandbox nokia-snake.c)

include_directories(
    ..
    ../..
    ../../core
    ../../bootloader/targets/furi-hal-include
    ../../bootloader/targets/f7/furi-hal
    ../../lib/STM32CubeWB/Drivers/CMSIS/Device/ST/STM32WBxx/Include
    ../../lib/STM32CubeWB/Drivers/STM32WBxx_HAL_Driver/Inc
    ../../lib/STM32CubeWB/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ../../lib/STM32CubeWB/Middlewares/Third_Party/FreeRTOS/Source/include
    ../../lib/STM32CubeWB/Middlewares/Third_Party/FreeRTOS/Source
    ../../assets/compiled
    ../../lib/u8g2
)
