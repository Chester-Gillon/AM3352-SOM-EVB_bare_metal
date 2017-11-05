project (ethernet_passthrough C ASM)

include_directories ("${AM3352_SOM_platform_SOURCE_DIR}")
include_directories ("${STARTERWARE_ROOT}/include")
include_directories ("${STARTERWARE_ROOT}/include/hw")
include_directories ("${STARTERWARE_ROOT}/include/armv7a/am335x")
add_executable (ethernet_passthrough.out "ethernet_passthrough_main.c")
set(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS}")
set_target_properties (ethernet_passthrough.out PROPERTIES LINK_FLAGS "-Wl,-Map,\"ethernet_passthrough.map\" -Wl,-T,\"${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds\" -Wl,--defsym,\"HEAPSIZE=0x100000\" -Wl,--defsym,\"SYSTEM_STACKSIZE=0x2000\" -Wl,--defsym,\"EXCEPTION_STACKSIZE=0x1000\" -Wl,--gc-sections")
set_target_properties (ethernet_passthrough.out PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds") 
TARGET_LINK_LIBRARIES (ethernet_passthrough.out utils uart_interrupts AM3352_SOM_platform system_config drivers c nosys)
                               
add_custom_command (OUTPUT app
                    COMMAND "${CCS_INSTALL_ROOT}/ccsv7/utils/tiobj2bin/tiobj2bin" ethernet_passthrough.out ethernet_passthrough.bin "${TIOBJ2BIN_HELPERS}"
                    COMMAND "${STARTERWARE_ROOT}/tools/ti_image/tiimage" 0x80000000 NONE ethernet_passthrough.bin app
                    DEPENDS ethernet_passthrough.out
                    COMMENT "Generating ethernet_passthrough app")
add_custom_target (ethernet_passthrough_app ALL
                   DEPENDS app ethernet_passthrough.out)                                       