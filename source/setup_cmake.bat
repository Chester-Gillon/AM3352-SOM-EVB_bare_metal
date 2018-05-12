rem Set up the cmake build environment under Windows, to a clean state
@echo off

rem Paths to CCS, the GCC ARM compiler for building the software and AM335x Starterware.
rem The path to the TI ARM compiler is required for generating the binary files.
set TI_INSTALL_ROOT=c:/ti
set CCS_INSTALL_ROOT=%TI_INSTALL_ROOT%/ccs800
set ECLIPSE_EXE="%CCS_INSTALL_ROOT%/ccsv8/eclipse/eclipse"
set ARM_GCC_ROOT=%CCS_INSTALL_ROOT%/ccsv8/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major-win32
set CG_TOOL_ROOT=%CCS_INSTALL_ROOT%/ccsv8/tools/compiler/ti-cgt-arm_18.1.1.LTS
set CMAKE_MAKE_PROGRAM=%CCS_INSTALL_ROOT%/ccsv8/utils/bin/gmake.exe
set STARTERWARE_ROOT=%TI_INSTALL_ROOT%/AM335X_StarterWare_02_00_01_01

rem Get the absolute path of the workspace root directory, which is the parent directory of this script.
for %%i in ("%~dp0.") do SET "SCRIPT_PATH=%%~fi"
pushd "%SCRIPT_PATH%\.."
set WORKSPACE_PATH=%CD%
popd

rem Create symbolic links to the GCC ARM cross-compilers, without the prefix.
rem This matches the executable names the "CDT Cross GCC Build-in Compiler Settings" runs to get the compiler specs
rem to find the include paths for the GCC ARM compiler.
rem
rem The gcc and g++ programs locate include files via paths relative to the executable.
rem Under windows a relative path on a symlink file is relative to the symlink location rather than target.
rem Therefore, create a directory structure so that the relative paths are valid.
mkdir %WORKSPACE_PATH%\gcc-arm-none-eabi\bin
set ARM_GCC_PREFIX=%ARM_GCC_ROOT%\bin\arm-none-eabi-
mklink /d "%WORKSPACE_PATH%\gcc-arm-none-eabi\lib" "%ARM_GCC_ROOT%\lib"
mklink /d "%WORKSPACE_PATH%\gcc-arm-none-eabi\arm-none-eabi" "%ARM_GCC_ROOT%\arm-none-eabi"
mklink "%WORKSPACE_PATH%\gcc-arm-none-eabi\bin\gcc.exe" "%ARM_GCC_PREFIX%gcc.exe" 
mklink "%WORKSPACE_PATH%\gcc-arm-none-eabi\bin\g++.exe" "%ARM_GCC_PREFIX%g++.exe" 

rem Create a batch file used to launch ccstudio such that:
rem a. STARTERWARE_ROOT environment variable set which is referenced in the "Paths and Symbols" to obtain the include files
rem    for StartWare for use by the Eclipse indexer.
rem b. The symbolic links to the GCC ARM gcc and g++ are first in the path, so that the "CDT Cross GCC Build-in Compiler Settings"
rem    gets the include files for the GCC ARM gcc rather than the native GCC.
rem c. Start with this project workspace selected, rather than propmting the user.
set LAUNCH_SCRIPT=%WORKSPACE_PATH%/launch_ccstudio.bat
echo @echo off> %LAUNCH_SCRIPT%
echo set STARTERWARE_ROOT=%STARTERWARE_ROOT%>> %LAUNCH_SCRIPT%
echo set PATH=%WORKSPACE_PATH%\gcc-arm-none-eabi\bin;%PATH%>> %LAUNCH_SCRIPT%
echo start %CCS_INSTALL_ROOT%\ccsv8\eclipse\ccstudio -data %WORKSPACE_PATH%>> %LAUNCH_SCRIPT%

rmdir /s /q %WORKSPACE_PATH%\build\Release
rmdir /s /q %WORKSPACE_PATH%\build\Debug

mkdir %WORKSPACE_PATH%\build\Release || goto :error
mkdir %WORKSPACE_PATH%\build\Debug || goto :error

pushd %WORKSPACE_PATH%\build\Release
cmake -G "Unix Makefiles" -DPLATFORM_TYPE=Release -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" ..\..\source
popd
pushd %WORKSPACE_PATH%\build\Debug
cmake -G "Unix Makefiles" -DPLATFORM_TYPE=Debug -DCCS_INSTALL_ROOT="%CCS_INSTALL_ROOT%" -DARM_GCC_ROOT="%ARM_GCC_ROOT%" -DSTARTERWARE_ROOT="%STARTERWARE_ROOT%" -DCG_TOOL_ROOT="%CG_TOOL_ROOT%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%" ..\..\source
popd

goto :end

:error
echo "Setup of cmake failed"

:end