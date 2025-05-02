#include "uart.h"
#include "printf.h"
#include "stdint.h"

/**
 * common exception handler
 */

 extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
 {
    // will branch here in the case of an exception, will add prints in next part


    // just to get rid of unused parameter warnings
     (void)type;
     (void)esr;
     (void)elr;
     (void)spsr;
     (void)far;
 
     while (1);
 }
 
