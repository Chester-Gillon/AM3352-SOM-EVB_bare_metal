@******************************************************************************
@
@ startup_ARMCA8.S - Init code routine for Cortex A8 cores and Linaro baremetal
@
@******************************************************************************
@
@ Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
@  (Rafael de Souza - 2014) Added VFP and NEON initialization routines
@
@
@  Redistribution and use in source and binary forms, with or without
@  modification, are permitted provided that the following conditions
@  are met:
@
@    Redistributions of source code must retain the above copyright
@    notice, this list of conditions and the following disclaimer.
@
@    Redistributions in binary form must reproduce the above copyright
@    notice, this list of conditions and the following disclaimer in the
@    documentation and/or other materials provided with the
@    distribution.
@
@    Neither the name of Texas Instruments Incorporated nor the names of
@    its contributors may be used to endorse or promote products derived
@    from this software without specific prior written permission.
@
@  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
@  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
@  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
@  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
@  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
@  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
@  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
@  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
@  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
@  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
@  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@
@******************************************************************************

@****************************** Global Symbols*******************************
        .global Entry
        .global IRQHandler
        .global FIQHandler
        .global AbortHandler
        .global SVC_Handler
        .global UndefInstHandler
        .global CPUAbortHandler
        .global __stack
        .global __exception_stack

@************************ Internal Definitions ******************************
@
@ Define the fixed stack sizes for different exception modes.
@
        .set  UND_STACK_SIZE, 0x8
        .set  ABT_STACK_SIZE, 0x8
        .set  FIQ_STACK_SIZE, 0x8
        .set  SVC_STACK_SIZE, 0x8

@
@ to set the mode bits in CPSR for different modes
@        
        .set  MODE_USR, 0x10            
        .set  MODE_FIQ, 0x11
        .set  MODE_IRQ, 0x12
        .set  MODE_SVC, 0x13
        .set  MODE_ABT, 0x17
        .set  MODE_UND, 0x1B
        .set  MODE_SYS, 0x1F            

        .equ  I_F_BIT, 0xC0               

@**************************** Code Section ***********************************
        .text

@
@ This code is assembled for ARM instructions
@
        .code 32

@******************************************************************************
@
@******************************************************************************
@
@ The reset handler sets up the stack pointers for all the modes. The FIQ and
@ IRQ shall be disabled during this. Then clears the BSS section, enters the  
@ main function. 

Entry:
@
@ Set up the Stack for Undefined mode
@
         LDR   r0, =__exception_stack          @ Read the stack address
         MSR   cpsr_c, #MODE_UND|I_F_BIT       @ switch to undef  mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0, r0, #UND_STACK_SIZE         @ give stack space
@
@ Set up the Stack for abort mode
@
         MSR   cpsr_c, #MODE_ABT|I_F_BIT       @ Change to abort mode
         MOV   sp, r0                          @ write the stack pointer
         SUB   r0,r0, #ABT_STACK_SIZE          @ give stack space
@
@ Set up the Stack for FIQ mode
@
         MSR   cpsr_c, #MODE_FIQ|I_F_BIT       @ change to FIQ mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0,r0, #FIQ_STACK_SIZE          @ give stack space
@
@ Set up the Stack for SVC mode
@
         MSR   cpsr_c, #MODE_SVC|I_F_BIT       @ change to SVC mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0,r0, #SVC_STACK_SIZE          @ give stack space
@
@ Set up the Stack for IRQ mode, for the remainder of the exception stack
@
         MSR   cpsr_c, #MODE_IRQ|I_F_BIT       @ change to IRQ mode
         MOV   sp,r0                           @ write the stack pointer

@ Set up the Stack for USer/System mode
@
         LDR   r0, =__stack                    @ Read the stack address
         MSR   cpsr_c, #MODE_SYS|I_F_BIT       @ change to system mode
         MOV   sp,r0                           @ write the stack pointer

@
@ Set up the Vector Base Address Regsiter
@
		LDR   r0, = __isr_vector
		MCR   p15, 0, r0, c12, c0, 0     @ Write VBAR Register

@ Invalidate and Enable Branch Prediction
         MOV     r0, #0
         MCR     p15, #0, r0, c7, c5, #6
         ISB
         MRC     p15, #0, r0, c1, c0, #0
         ORR     r0, r0, #0x00000800
         MCR     p15, #0, r0, c1, c0, #0

.if __ARM_PCS_VFP == 1
@
@ Enable NEON extensions and the VFP. Must be done before entering user mode.
@
        MRC   p15, #0, r3, c1, c0, #2    @ Read CPACR
		ORR   r3, r3, #0x00F00000        @ Enable access to CP10 and CP11
		MCR   p15, #0, r3, c1, c0, #2    @ Write CPACR
		MOV   r3, #0
      	MOV   r0,#0x40000000
        FMXR  FPEXC,r0                   @ Set FPEXC bit 30 to enable VFP
		MCR   p15, #0, r3, c7, c5, #4    @ flush prefetch buffer because of FMXR above
.endif
		
.if USERMODE == 1
@
@ Change to user mode. It allows for a stack smaller than 64kB.
@
        MSR   cpsr_c, #MODE_USR|I_F_BIT  @ change to user mode
.endif

@
@ The BSS section is cleared by the runtime support library
@

@
@ Enter the runtime support library, which will call the main function.
@ _start will set the stack pointer for the current mode (system) to the value of __stack
@

Enter_BootLoader:
         LDR   r10, = _start              @ Get the address of _start
         MOV   lr,pc                     @ Dummy return 
         BX    r10                       @ Branch to main 
         SUB   pc, pc, #0x08             @ looping   
    
@
@ Set the Interrupt vector table here
@

    .section .isr_vector
    .align 4
    .globl  __isr_vector
__isr_vector:
        LDR   pc, [pc,#24]       @ 0x00 Reset
        LDR   pc, [pc,#24]       @ 0x04 Undefined Instruction
        LDR   pc, [pc,#24]       @ 0x08 Supervisor Call
        LDR   pc, [pc,#-8]       @ 0x0C Prefetch Abort
        LDR   pc, [pc,#24]       @ 0x10 Data Abort
        LDR   pc, [pc,#-8]       @ 0x14 Not used
        LDR   pc, [pc,#24]       @ 0x18 IRQ interrupt
        LDR   pc, [pc,#24]       @ 0x1C FIQ interrupt
        .long  Entry
        .long  UndefInstHandler
        .long  SVC_Handler
        .long  0
        .long  AbortHandler
        .long  0
        .long  IRQHandler
        .long  FIQHandler
   
@
@ End of the file
@
       .end
    
