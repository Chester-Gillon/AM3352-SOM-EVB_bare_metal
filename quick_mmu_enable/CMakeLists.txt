project (quick_mmu_enable C ASM)

include_directories ("${AM3352_SOM_platform_SOURCE_DIR}")
include_directories ("${STARTERWARE_ROOT}/include")
include_directories ("${STARTERWARE_ROOT}/include/armv7a")
add_executable (quick_mmu_enable.out "quick_mmu_enable.c"
                                     "${AM3352_SOM_platform_SOURCE_DIR}/startup_ARMCA8.S")
set(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS}")
set_target_properties (quick_mmu_enable.out PROPERTIES LINK_FLAGS "-Wl,-Map,\"quick_mmu_enable.map\" -Wl,-T,\"${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds\" -Wl,--defsym,\"HEAPSIZE=0x0\" -Wl,--defsym,\"STACKSIZE=0x2000\" -Wl,--gc-sections")
set_target_properties (quick_mmu_enable.out PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AM335x.lds") 
TARGET_LINK_LIBRARIES (quick_mmu_enable.out system_config utils AM3352_SOM_platform drivers c nosys)
                             
add_custom_command (OUTPUT MLO
                    COMMAND "${CCS_INSTALL_ROOT}/ccsv7/utils/tiobj2bin/tiobj2bin" quick_mmu_enable.out quick_mmu_enable.bin "${TIOBJ2BIN_HELPERS}"
                    COMMAND "${STARTERWARE_ROOT}/tools/ti_image/tiimage" 0x40300000 MMCSD quick_mmu_enable.bin MLO
                    DEPENDS quick_mmu_enable.out
                    COMMENT "Generating quick_mmu_enable MLO")
add_custom_target (quick_mmu_enable_app ALL
                   DEPENDS MLO quick_mmu_enable.out)