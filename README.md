# AM3352-SOM-EVB_bare_metal
Experiments for an Olimex AM3352-SOM-EVB (https://www.olimex.com/Products/SOM/AM3352/AM3352-SOM-EVB/open-source-hardware)
which don't involve Linux.

The programs have been developed using:
- CCS 6.1.1
- TI ARM Compiler v5.2.6
- AM335X StarterWare 02.00.01.01
  STARTERWARE_SW_ROOT, as a Linked Resource path variable at the workspace level,
  should be set to the root directory of the StarterWare installation


@todo Intermittent failures seen:
1) On some power-cycles the booter loader has been seen to hang after reporting the following:
Copying application image from MMC/SD card to RAM
Jumping to StarterWare Application...

After two failures, attaching the debugger showed the bootloader had read the image header but then hung attempting
to read the image from SD card. Was hung inside the HSMMCSDStatusGet() function.

