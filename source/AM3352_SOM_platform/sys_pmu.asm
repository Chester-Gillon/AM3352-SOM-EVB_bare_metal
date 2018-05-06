/*
  @file sys_pmu.asm
  @date 11 Apr 2015
  @author Chester Gillon
  @brief Minimal assembler functions to allow the Cycle Counter in the Cortex-A8 PMU to be used
*/

/* Enable to Cycle Count to start free-running
   Note this resets the Cycle Count */
    .text
    .global enable_cycle_count

enable_cycle_count:

        stmfd sp!, {r0}
        mrc   p15, #0, r0, c9, c12, #0
        orr   r0,  r0, #7
        mcr   p15, #0, r0, c9, c12, #0
        mov   r0,  #1 << 31
        mcr   p15, #0, r0, c9, c12, #1
        ldmfd sp!, {r0}
        bx    lr

/* Return the current value of the Cycle Counter */
        .global pmu_get_cycle_count
pmu_get_cycle_count:

        mrc   p15, #0, r0, c9, c13, #0
        bx    lr



