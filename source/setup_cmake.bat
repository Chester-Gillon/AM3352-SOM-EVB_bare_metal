rem Set up the cmake build environment under Windows, to a clean state

set TI_INSTALL_ROOT=c:/ti
set CCS_INSTALL_ROOT=%TI_INSTALL_ROOT%/ccs800
set ECLIPSE_EXE="%CCS_INSTALL_ROOT%/ccsv8/eclipse/eclipse"
set ARM_GCC_ROOT=%CCS_INSTALL_ROOT%/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major-win32
set CG_TOOL_ROOT=%CCS_INSTALL_ROOT%/ccsv8/tools/compiler/ti-cgt-arm_18.1.1.LTS
set CMAKE_MAKE_PROGRAM=%CCS_INSTALL_ROOT%/ccsv8/utils/bin/gmake.exe
set STARTERWARE_ROOT=%TI_INSTALL_ROOT%/AM335X_StarterWare_02_00_01_01

rmdir /s /q ..\Release
rmdir /s /q ..\Debug

mkdir ..\Release || goto :error
mkdir ..\Debug || goto :error

cd ../Release
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" -DCMAKE_ECLIPSE_EXECUTABLE=%ECLIPSE_EXE% ..\src
cd ../Debug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" -DCMAKE_ECLIPSE_EXECUTABLE=%ECLIPSE_EXE% ..\src

cd ../src
goto :end

:error
echo "Setup of cmake failed"

:end