#! /bin/bash
# Set up the cmake build environment under Linux, to a clean state

CCS_INSTALL_ROOT="${CCS_INSTALL_ROOT:-${HOME}/ti/ccs800}"
ECLIPSE_EXE="${CCS_INSTALL_ROOT}/ccsv8/eclipse/eclipse"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${CCS_INSTALL_ROOT}/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major}"
STARTERWARE_ROOT="${STARTERWARE_ROOT:-${HOME}/ti/AM335X_StarterWare_02_00_01_01}"

rm -rf ../Release
rm -rf ../Debug
mkdir ../Release
mkdir ../Debug

cd ../Release
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} -DCMAKE_ECLIPSE_EXECUTABLE=${ECLIPSE_EXE} ../src
cd ../Debug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} -DCMAKE_ECLIPSE_EXECUTABLE=${ECLIPSE_EXE} ../src

