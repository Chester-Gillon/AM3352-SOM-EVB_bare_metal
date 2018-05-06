#! /bin/bash
# Set up the cmake build environment under Linux, to a clean state

CCS_INSTALL_ROOT="${CCS_INSTALL_ROOT:-${HOME}/ti/ccs800}"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${CCS_INSTALL_ROOT}/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major}"
STARTERWARE_ROOT="${STARTERWARE_ROOT:-${HOME}/ti/AM335X_StarterWare_02_00_01_01}"

rm -rf ../build
mkdir ../build
mkdir ../build/Release
mkdir ../build/Debug

pushd ../build/Release
cmake -G "Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../../source
popd
pushd ../build/Debug
cmake -G "Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../../source
popd

