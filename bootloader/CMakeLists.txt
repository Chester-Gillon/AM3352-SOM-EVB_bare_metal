project (bootloader C ASM)

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories ("${AM3352_SOM_platform_SOURCE_DIR}")
include_directories ("${STARTERWARE_ROOT}/include")
include_directories ("${STARTERWARE_ROOT}/include/hw")
include_directories ("${STARTERWARE_ROOT}/include/armv7a/am335x")
include_directories ("${STARTERWARE_ROOT}/bootloader/include")
include_directories ("${STARTERWARE_ROOT}/third_party/fatfs/src")
include_directories ("${STARTERWARE_ROOT}/mmcsdlib/include")
add_executable (bootloader.out "bl_platform.c"
                               "${STARTERWARE_ROOT}/bootloader/src/bl_main.c"
                               "${STARTERWARE_ROOT}/bootloader/src/bl_hsmmcsd.c"
                               "${STARTERWARE_ROOT}/bootloader/src/bl_copy.c"
                               "${AM3352_SOM_platform_SOURCE_DIR}/startup_ARMCA8.S")
set(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS} -DAM3352_SOM -Dam335x -DMMCSD -Dam3352")
# The default max-page-size in the gcc-arm-none-eabi-4_9-2015q3 linker is 0x8000.
# This causes the generated ELF bootload.out file to have ELF program sections which are aligned for 0x8000 bytes
# which causes the CCS debugger to attempt to write to addresses less than the start address of 0x402F0400 when
# attempting to download the program, which fails since the 0x402F0000 ... 0x402F03FF is not accessable in an AM3352.
# To prevent an error when downloading the program, the max-page-size is set to 0x400 which matches the alignment of the start address.
# See https://e2e.ti.com/support/development_tools/code_composer_studio/f/81/t/598781
set_target_properties (bootloader.out PROPERTIES LINK_FLAGS "-Wl,-Map,\"bootloader.map\" -Wl,-T,\"${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds\" -Wl,--defsym,\"HEAPSIZE=0x0\" -Wl,--defsym,\"STACKSIZE=0x2000\" -Wl,--gc-sections -z max-page-size=0x400")
set_target_properties (bootloader.out PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds") 
TARGET_LINK_LIBRARIES (bootloader.out utils AM3352_SOM_platform mmcsdlib fatfs drivers system_config c nosys)
                             
add_custom_command (OUTPUT MLO
                    COMMAND "${CCS_INSTALL_ROOT}/ccsv7/utils/tiobj2bin/tiobj2bin" bootloader.out bootloader.bin "${TIOBJ2BIN_HELPERS}"
                    COMMAND "${STARTERWARE_ROOT}/tools/ti_image/tiimage" 0x402F0400 MMCSD bootloader.bin MLO
                    DEPENDS bootloader.out
                    COMMENT "Generating bootloader MLO")
add_custom_target (bootloader_app ALL
                   DEPENDS MLO bootloader.out)                         