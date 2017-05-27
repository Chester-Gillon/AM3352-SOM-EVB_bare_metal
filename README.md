# AM3352-SOM-EVB_bare_metal
Experiments for an Olimex AM3352-SOM-EVB (https://www.olimex.com/Products/SOM/AM3352/AM3352-SOM-EVB/open-source-hardware)
which don't involve Linux.

The programs have been developed using:
- CCS 7.1
- GCC ARM Compiler 4_9-2015q3
- AM335X StarterWare 02.00.01.01
- cmake v3.5.1


@todo Intermittent failures seen:
1) On some power-cycles the booter loader has been seen to hang after reporting the following:
Copying application image from MMC/SD card to RAM
Jumping to StarterWare Application...

After two failures, attaching the debugger showed the bootloader had read the image header but then hung attempting
to read the image from SD card. Was hung inside the HSMMCSDStatusGet() function.

