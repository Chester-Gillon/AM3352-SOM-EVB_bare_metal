set (CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/gcc_arm_toolchain.cmake")

project (AM3352-SOM-EVB C)
cmake_minimum_required (VERSION 2.8)

add_subdirectory (AM3352_SOM_platform)
add_subdirectory (sdram_test)
add_subdirectory (ethernet_passthrough)
add_subdirectory (quick_mmu_enable)
add_subdirectory (bootloader)
