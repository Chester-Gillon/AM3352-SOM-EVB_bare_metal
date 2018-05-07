#! /bin/bash
# Set up the cmake build environment under Linux, to a clean state

CCS_INSTALL_ROOT="${CCS_INSTALL_ROOT:-${HOME}/ti/ccs800}"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${CCS_INSTALL_ROOT}/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major}"
STARTERWARE_ROOT="${STARTERWARE_ROOT:-${HOME}/ti/AM335X_StarterWare_02_00_01_01}"

platforms="Debug Release"
for platform in ${platforms}
do
   build_dir=../build/${platform}
   rm -rf ${build_dir}
   mkdir ${build_dir}
   pushd ${build_dir}
   cmake -G "Unix Makefiles" -DPLATFORM_TYPE=${platform} -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../../source
   popd
done

