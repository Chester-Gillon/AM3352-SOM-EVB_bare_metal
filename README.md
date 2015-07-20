# AM3352-SOM-EVB_bare_metal
Experiments for an Olimex AM3352-SOM-EVB (https://www.olimex.com/Products/SOM/AM3352/AM3352-SOM-EVB/open-source-hardware)
which don't involve Linux.

The programs have been developed using:
- CCS 6.1
- TI ARM Compiler v5.2.4
- AM335X StarterWare 02.00.01.01
  STARTERWARE_SW_ROOT, as a Linked Resource path variable at the workspace level,
  should be set to the root directory of the StarterWare installation

@todo The bootloader works when compiled for Debug, but when compiled for Release fails with:
StarterWare AM3352 SOM Boot Loader

 Unable to open application file

Need to determine if there is a compiler bug or something else.
Temporary changes made to investigate bootloader failure:
a) Leave compiler version as v5.2.4 and changing bl_hsmmcsd.c source file to be compiled with Optimization Level 1, rather than 2
   allows the bootloader to run when compiled for release

b) Change compiler version to v5.1.12 and bootloader still failed to run when compiled for release

c) Change compiler version to v5.0.11 and bootloader run when compiled for release

Inspecting the bl_hsmmcsd.asm assembler list shows that the v5.0.11 compiler is producing the following warning, which v5.1.12 and v5.2.4 don't produce, which may invalidate the comparison:
;******************************************************************************
;*                                                                            *
;* Using -g (debug) with optimization (-o2) may disable key optimizations!    *
;*                                                                            *
;******************************************************************************
 
