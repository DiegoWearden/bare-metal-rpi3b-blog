#include "arm/sysregs.h"
#include "mm.h"
#include "vm.h"
 
 .section ".text.boot"
 _start:	
     mrs x0, mpidr_el1
     and x0, x0, #0xFF        // Check processor id
     cbz x0, primary           // Continue for primary CPU
     b proc_hang              // Hang for all non-primary CPUs
 
 proc_hang:
     wfe
     b proc_hang
 
 primary: 
    add x1, x1, #LOW_MEMORY
    mov sp, x1
    bl pickKernelStack
    msr sp_el1, x0

     // Disable coprocessor traps in EL2
     mov     x0, #0x33FF
     msr     cptr_el2, x0
     msr     hstr_el2, xzr
     mrs x0, CPACR_EL1         // Read the current value of CPACR_EL1
     orr x0, x0, #(0b01 << 20) // Set bits 20-21 to enable full access to FP/SIMD
     msr CPACR_EL1, x0         // Write the value back to CPACR_EL1
 
     // Configure EL2 (Hypervisor)
     ldr x0, =HCR_VALUE
     msr hcr_el2, x0
 
     ldr     x0, =_vectors
     msr     vbar_el1, x0
 
     // Set up SPSR_EL2 to transition to EL1h
     mov x0, #0x3c4
     msr spsr_el2, x0
 
     // Set ELR_EL2 to point to el1_entry
     adr x0, el1_entry
     msr elr_el2, x0
 
     // Transition to EL1
     eret
 
 el1_entry:
     // Clear BSS section
     adr    x0, __bss_start
     adr    x1, __bss_end
     sub    x1, x1, x0
     bl     memzero
 
     // Set up the stack pointer (SP) for EL1
     mov    sp, #LOW_MEMORY
     bl pickKernelStack
     mov    sp, x0
 
     bl    primary_kernel_init
     b     proc_hang        // should never come here
 
 setup_el1_for_secondary:

    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #SECTION_SIZE
    mul x1, x1, x0
    add x1, x1, #LOW_MEMORY
    mov sp, x1

    // Set up stack pointer for EL1 for secondary cores
    bl pickKernelStack
    msr sp_el1, x0

     // Disable coprocessor in EL2
     mov     x0, #0x33FF
     msr     cptr_el2, x0
     msr     hstr_el2, xzr
     mov     x0, #(3 << 20)
     msr     cpacr_el1, x0
 
     // Configure EL2 (Hypervisor)
     ldr x0, =HCR_VALUE
     msr hcr_el2, x0

    ldr     x0, =_vectors
    msr     vbar_el1, x0
 
     // Set up SPSR_EL2 to transition to EL1h
     ldr x0, =SPSR_VALUE
     msr spsr_el2, x0
 
     // Set ELR_EL2 to point to secondary_kernel_main
     adr x0, secondary_kernel_main
     msr elr_el2, x0
 
     // Transition to EL1
     eret
 
 secondary_kernel_main:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #SECTION_SIZE
    mul x1, x1, x0
    add x1, x1, #LOW_MEMORY
    mov sp, x1
    bl pickKernelStack
    mov sp, x0
    bl secondary_kernel_init
    b proc_hang
 
.global wake_up_cores
wake_up_cores:
    
    mov x0, #0
    adr x0, setup_el1_for_secondary 
    ldr x4, =(VA_START)
    sub x0, x0, x4

    dsb sy
    isb

    mov x1, #0xe0
    str x0, [x1]
    mov x1, #0xe8
    str x0, [x1]
    mov x1, #0xf0
    str x0, [x1]

    // we wrote to addresses 0xe0, 0xe8, and 0xf0, we must now write them back to memory since 
    // primary core has data cache on at this point
    movz x0, #0xe0
    dc cvac, x0
    movz x0, #0xe8
    dc cvac, x0
    movz x0, #0xf0
    dc cvac, x0
    
    dsb sy
    isb
    
    sev
    
    ret
 
     // important, code has to be properly aligned
     .align 11
 _vectors:
     // synchronous
     .align  7
     mov     x0, #0
     mrs     x1, esr_el1
     mrs     x2, elr_el1
     mrs     x3, spsr_el1
     mrs     x4, far_el1
     b       exc_handler
 
     // IRQ
     .align  7
     mov     x0, #1
     mrs     x1, esr_el1
     mrs     x2, elr_el1
     mrs     x3, spsr_el1
     mrs     x4, far_el1
     b       exc_handler
 
     // FIQ
     .align  7
     mov     x0, #2
     mrs     x1, esr_el1
     mrs     x2, elr_el1
     mrs     x3, spsr_el1
     mrs     x4, far_el1
     b       exc_handler
 
     // SError
     .align  7
     mov     x0, #3
     mrs     x1, esr_el1
     mrs     x2, elr_el1
     mrs     x3, spsr_el1
     mrs     x4, far_el1
     b       exc_handler