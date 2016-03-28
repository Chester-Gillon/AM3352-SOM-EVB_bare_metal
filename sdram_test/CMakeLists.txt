project (sdram_test C ASM)

include_directories ("${AM3352_SOM_platform_SOURCE_DIR}")
include_directories ("${STARTERWARE_ROOT}/include")
include_directories ("${STARTERWARE_ROOT}/include/hw")
include_directories ("${STARTERWARE_ROOT}/include/armv7a")
add_executable (sdram_test.out "sdram_test_main.c"
                               "${AM3352_SOM_platform_SOURCE_DIR}/startup_ARMCA8.S")
set(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS}")
set_target_properties (sdram_test.out PROPERTIES LINK_FLAGS "-Wl,-Map,\"sdram_test.map\" -Wl,-T,\"${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds\" -Wl,--defsym,\"HEAPSIZE=0x0\" -Wl,--defsym,\"STACKSIZE=0x2000\" -Wl,--gc-sections")
set_target_properties (sdram_test.out PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds") 
TARGET_LINK_LIBRARIES (sdram_test.out system_config utils AM3352_SOM_platform drivers c nosys)
                             
add_custom_command (OUTPUT app
                    COMMAND "${CCS_INSTALL_ROOT}/ccsv6/utils/tiobj2bin/tiobj2bin" sdram_test.out sdram_test.bin "${TIOBJ2BIN_HELPERS}"
                    COMMAND "${STARTERWARE_ROOT}/tools/ti_image/tiimage" 0x40300000 NONE sdram_test.bin app
                    DEPENDS sdram_test.out
                    COMMENT "Generating sdram_test app")
add_custom_target (sdram_test_app ALL
                   DEPENDS app sdram_test.out)