#! /bin/bash
# Set up the cmake build environment under Linux, to a clean state

# Paths to CCS, the GCC ARM compiler and AM335x Starterware
CCS_INSTALL_ROOT="${CCS_INSTALL_ROOT:-${HOME}/ti/ccs800}"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${CCS_INSTALL_ROOT}/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major}"
STARTERWARE_ROOT="${STARTERWARE_ROOT:-${HOME}/ti/AM335X_StarterWare_02_00_01_01}"

# Get the absolute path of the workspace root directory, which is the parent directory of this script.
SCRIPT=$(readlink -f $0)
SCRIPT_PATH=`dirname ${SCRIPT}`
WORKSPACE_PATH=$(readlink -f ${SCRIPT_PATH}/..)

# Create symbolic links to the GCC ARM cross-compilers, without the prefix.
# This matches the executable names the "CDT Cross GCC Build-in Compiler Settings" runs to get the compiler specs
# to find the include paths for the GCC ARM compiler
ARM_GCC_PREFIX=${ARM_GCC_ROOT}/bin/arm-none-eabi-
ln -sf ${ARM_GCC_PREFIX}gcc ${WORKSPACE_PATH}/gcc
ln -sf ${ARM_GCC_PREFIX}g++ ${WORKSPACE_PATH}/g++

# Create a script used to launch ccstudio such that:
# a. STARTERWARE_ROOT environment variable set which is referenced in the "Paths and Symbols" to obtain the include files
#    for StartWare for use by the Eclipse indexer.
# b. The symbolic links to the GCC ARM gcc and g++ are first in the path, so that the "CDT Cross GCC Build-in Compiler Settings"
#    gets the include files for the GCC ARM gcc rather than the native GCC.
# c. Start with this project workspace selected, rather than propmting the user.
LAUNCH_SCRIPT=${WORKSPACE_PATH}/launch_ccstudio.sh
cat << END_TEXT > ${LAUNCH_SCRIPT}
export STARTERWARE_ROOT=${STARTERWARE_ROOT}
PATH=${WORKSPACE_PATH}:${PATH}
${CCS_INSTALL_ROOT}/ccsv8/eclipse/ccstudio -data ${WORKSPACE_PATH}&
END_TEXT

chmod +x ${LAUNCH_SCRIPT}

platforms="Debug Release"
for platform in ${platforms}
do
   build_dir=${WORKSPACE_PATH}/build/${platform}
   rm -rf ${build_dir}
   mkdir ${build_dir}
   pushd ${build_dir}
   cmake -G "Unix Makefiles" -DPLATFORM_TYPE=${platform} -DCCS_INSTALL_ROOT=${CCS_INSTALL_ROOT} -DARM_GCC_ROOT=${ARM_GCC_ROOT} -DSTARTERWARE_ROOT=${STARTERWARE_ROOT} ../../source
   popd
done

