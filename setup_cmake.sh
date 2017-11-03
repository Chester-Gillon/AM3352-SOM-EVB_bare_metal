#! /bin/bash
# Set up the cmake build environment under Linux, to a clean state

CCS_INSTALL_ROOT="${CCS_INSTALL_ROOT:-${HOME}/ti/ccs730}"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${CCS_INSTALL_ROOT}/ccsv7/tools/compiler/gcc-arm-none-eabi-6-2017-q1-update}"
STARTERWARE_ROOT="${STARTERWARE_ROOT:-${HOME}/ti/AM335X_StarterWare_02_00_01_01}"

rm -rf ../Release
rm -rf ../Debug
mkdir ../Release
mkdir ../Debug

cd ../Release
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../src
cd ../Debug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../src

