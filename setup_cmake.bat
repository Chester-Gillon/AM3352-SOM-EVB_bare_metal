rem Set up the cmake build environment under Linux, to a clean state

set CCS_INSTALL_ROOT=c:/ti_ccs6_1
set ARM_GCC_ROOT=%CCS_INSTALL_ROOT%/ccsv6/tools/compiler/gcc-arm-none-eabi-4_8-2014q3
set CG_TOOL_ROOT=%CCS_INSTALL_ROOT%/ccsv6/tools/compiler/ti-cgt-arm_5.2.7
set CMAKE_MAKE_PROGRAM=%CCS_INSTALL_ROOT%/ccsv6/utils/bin/gmake.exe
set STARTERWARE_ROOT=%CCS_INSTALL_ROOT%/AM335X_StarterWare_02_00_01_01

rmdir /s /q ..\Release
rmdir /s /q ..\Debug

mkdir ..\Release
mkdir ..\Debug

cd ../Release
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" ..\src
cd ../Debug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" ..\src