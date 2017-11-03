project (AM3352_SOM_platform C ASM)

cmake_minimum_required (VERSION 2.8)

include_directories ("${STARTERWARE_ROOT}/include")
include_directories ("${STARTERWARE_ROOT}/include/hw")
include_directories ("${STARTERWARE_ROOT}/include/armv7a")
include_directories ("${STARTERWARE_ROOT}/include/armv7a/am335x")
include_directories ("${STARTERWARE_ROOT}/mmcsdlib/include")
include_directories ("${STARTERWARE_ROOT}/third_party/fatfs/src")
add_library (AM3352_SOM_platform platform_cpsw.c
                                 rtc.c
                                 uartConsole.c
                                 dmtimer.c
                                 platform_hs_mmcsd.c
                                 sysperf.c
                                 uart.c
                                 sys_pmu.asm)
set(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS}")

add_library (system_config "${STARTERWARE_ROOT}/system_config/armv7a/mmu.c"
                           "${STARTERWARE_ROOT}/system_config/armv7a/cache.c"
                           "${STARTERWARE_ROOT}/system_config/armv7a/am335x/device.c"
                           "${STARTERWARE_ROOT}/system_config/armv7a/gcc/cp15.S")

add_library (utils "${STARTERWARE_ROOT}/utils/uartStdio.c"
                   "${STARTERWARE_ROOT}/utils/ascii.c"
                   "${STARTERWARE_ROOT}/utils/consoleUtils.c")

add_library (drivers "${STARTERWARE_ROOT}/drivers/uart_irda_cir.c"
                     "${STARTERWARE_ROOT}/drivers/rtc.c"
                     "${STARTERWARE_ROOT}/drivers/dmtimer.c"
                     "${STARTERWARE_ROOT}/drivers/mdio.c"
                     "${STARTERWARE_ROOT}/drivers/cpsw.c"
                     "${STARTERWARE_ROOT}/drivers/phy.c"
                     "${STARTERWARE_ROOT}/drivers/hs_mmcsd.c")

add_library (mmcsdlib "${STARTERWARE_ROOT}/mmcsdlib/hs_mmcsdlib.c"
                      "${STARTERWARE_ROOT}/mmcsdlib/mmcsd_proto.c")

# Disable the generation of instructions to perform unaligned accesses for just the ff.c source file, since otherwise the GCC v6.3.1
# optimiser can generate an unaligned word load which causes the bootloader to fail with an abort
# (the bootloader runs with the MMU disabled which means unaligned accesses cause a data abort)
set_source_files_properties("${STARTERWARE_ROOT}/third_party/fatfs/src/ff.c" PROPERTIES COMPILE_FLAGS -mno-unaligned-access)
add_library (fatfs "${STARTERWARE_ROOT}/third_party/fatfs/src/ff.c"
                   "${STARTERWARE_ROOT}/third_party/fatfs/port/fat_mmcsd.c")
