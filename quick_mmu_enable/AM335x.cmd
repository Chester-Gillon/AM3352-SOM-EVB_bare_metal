/****************************************************************************/
/*  AM335x.cmd                                                              */
/*  Copyright (c) 2014  Texas Instruments Incorporated                      */
/*  Author: Rafael de Souza                                                 */
/*                                                                          */
/*    Description: This file is a sample linker command file that can be    */
/*                 used for linking programs built with the C compiler and  */
/*                 running the resulting .out file on an AM335x device.     */
/*                 Use it as a guideline.  You will want to                 */
/*                 change the memory layout to match your specific          */
/*                 target system.  You may want to change the allocation    */
/*                 scheme according to the size of your program.            */
/*                                                                          */
/****************************************************************************/
-e Entry

/* Suppress the following diagnostic due to a non-default entry point:
   warning #10063-D: entry-point symbol other than "_c_int00" specified:  "Entry"
*/
--diag_suppress=10063


/* SPECIFY THE SYSTEM MEMORY MAP */

MEMORY
{
        /* Lower part of 64kB L3 OCMC SRAM, excluding boot ROM Public Memory Map.
           This is to allow the boot ROM tracing vectors to be examined after
           load of this program. */
        IRAM_MEM        : org = 0x40300000  len = 0x0000B800
}

/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY */

SECTIONS
{
    /* Initialised sections */
    GROUP
    {
        .text:Entry
        .text
        .const
        .cinit
    } > IRAM_MEM

    /* Uninitialised sections */
    GROUP
    {
        .bss
                RUN_START(bss_start)
                RUN_END(bss_end)
        .stack
    } > IRAM_MEM HIGH
}
