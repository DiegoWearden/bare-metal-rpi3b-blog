#include "uart.h"
#include "printf.h"
#include "stdint.h"

/**
 * common exception handler
 */

 extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
 {
    printf_err("\n=== Exception Handler Triggered ===\n");
    printf_err("Type   : %lu\n", type);
    printf_err("ESR_EL1: 0x%lx\n", esr);
    printf_err("ELR_EL1: 0x%lx\n", elr);
    printf_err("SPSR_EL1: 0x%lx\n", spsr);
    printf_err("FAR_EL1: 0x%lx\n", far);
            
     // no return from exception for now
     while(1);
 }
