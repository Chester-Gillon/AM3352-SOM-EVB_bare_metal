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


2) Sometimes when starting the ethernet_passthrough program the LAN8710A Ethernet phy which is supposed to have Phy address 1 appeared
to be responding to different Phy addresses and the link wasn't established.
Added some code to read the Special Modes register from the Ethernet phy, which contains the Phy address, to determine if the problem
was the Phy getting set to the wrong address.

A working run is:
MDIOPhyAliveStatusGet() = 0x3
Phy 0 ID = 0x7c0f1  Special Modes = 0xe0
Phy 1 ID = 0x7c0f1  Special Modes = 0xe1


The different failures seen are:
Port 1 MAC address = 1c:ba:8c:9c:98:ba
Port 2 MAC address = 1c:ba:8c:9c:98:bc
MDIOPhyAliveStatusGet() = 0x5
Phy 0 ID = 0x7c0f1  Special Modes = 0xe0
Phy 2 ID = 0x7c0f1  Special Modes = 0x22
Phy 0 link Up    link speed 100M Full

Port 1 MAC address = 1c:ba:8c:9c:98:ba
Port 2 MAC address = 1c:ba:8c:9c:98:bc
MDIOPhyAliveStatusGet() = 0x9
Phy 0 ID = 0x7c0f1  Special Modes = 0xe0
Phy 3 ID = 0x7c0f1  Special Modes = 0x23
Phy 0 link Up    link speed 100M Full

Port 1 MAC address = 1c:ba:8c:9c:98:ba
Port 2 MAC address = 1c:ba:8c:9c:98:bc
MDIOPhyAliveStatusGet() = 0x9
Phy 0 ID = 0x7c0f1  Special Modes = 0xe0
Phy 3 ID = 0x7c0f1  Special Modes = 0x63
Phy 0 link Up    link speed 100M Full

Notes on the failures:
- Has only been seen when starting a debug session, or after asserting a system reset in the debugger.
  Has not been seen after a power-cycle or press of the reset button.

- After a failure, the Special Modes registers shows an incorrect Phy address and Mode have been set.
  This are supposed to be set by strapped options after a power up or reset input to the Phy.

Does a system reset from the debugger generate a short reset output?

