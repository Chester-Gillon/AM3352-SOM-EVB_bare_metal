# this one is important
set(CMAKE_SYSTEM_NAME "Generic")
include(CMakeForceCompiler)

set(TOOLCHAIN_COMPILER_IDENTIFIER "gcc-arm-none-eabi" CACHE STRING "compiler identifier" FORCE)

set(CMAKE_MAKE_PROGRAM "${CCS_INSTALL_ROOT}/ccsv7/utils/bin/gmake"                CACHE STRING "make program")


# Specify the cross compiler.
# Under Windows have to:
# - Add the .exe suffix to the compiler executables so they are found by CMake.
# - Set the ofd and hex utilities required by the tiobj2bin program. This means under Windows
#   CG_TOOL_ROOT must be set to point at a TI ARM compiler installation to get armhex and armofd
#   (under Linux tiobj2bin uses objcopy)
IF(${WIN32})
CMAKE_FORCE_C_COMPILER (${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc.exe GNU)
CMAKE_FORCE_CXX_COMPILER (${ARM_GCC_ROOT}/bin/arm-none-eabi-g++.exe GNU)
SET (CMAKE_ASM_COMPILER ${ARM_GCC_ROOT}/bin/arm-none-eabi-as.exe)
SET (TIOBJ2BIN_HELPERS "${CG_TOOL_ROOT}/bin/armofd.exe" "${CG_TOOL_ROOT}/bin/armhex.exe" "${CCS_INSTALL_ROOT}/ccsv8/utils/tiobj2bin/mkhex4bin.exe")
ELSE()
CMAKE_FORCE_C_COMPILER (${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER (${ARM_GCC_ROOT}/bin/arm-none-eabi-g++ GNU)
SET (CMAKE_ASM_COMPILER ${ARM_GCC_ROOT}/bin/arm-none-eabi-as)
SET (TIOBJ2BIN_HELPERS "")
ENDIF()

# skip compiler tests
set(CMAKE_ASM_COMPILER_WORKS 1) 
set(CMAKE_C_COMPILER_WORKS   1) 
set(CMAKE_CXX_COMPILER_WORKS 1) 
set(CMAKE_DETERMINE_ASM_ABI_COMPILED 1) 
set(CMAKE_DETERMINE_C_ABI_COMPILED   1) 
set(CMAKE_DETERMINE_CXX_ABI_COMPILED 1) 


# Add the default include and lib directories for tool chain
include_directories(${ARM_GCC_ROOT}/arm-none-eabi/include)

# set target environment
set(CMAKE_FIND_ROOT_PATH ${ARM_GCC_ROOT})

# specifiy target cpu flags

IF(PLATFORM_TYPE MATCHES Debug)
    set (OPTIMIZATION_C_FLAGS "-g")
    message ("Building for debug")
ELSE()
    set (OPTIMIZATION_C_FLAGS "-O3 -g")
    message ("Building for release")
ENDIF()
set(PLATFORM_CONFIG_C_FLAGS    "-Dgcc -ffunction-sections -fdata-sections -mcpu=cortex-a8 -mtune=cortex-a8 -march=armv7-a -marm -mfloat-abi=hard -mfpu=vfpv3 -Wall ${OPTIMIZATION_C_FLAGS}" CACHE STRING "platform config c flags")
set(PLATFORM_CONFIG_CXX_FLAGS  "${PLATFORM_CONFIG_C_FLAGS}"                                  CACHE STRING "platform config cxx flags")

# combine flags to C and C++ flags
SET(CMAKE_C_FLAGS "${PLATFORM_CONFIG_C_FLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${PLATFORM_CONFIG_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")


