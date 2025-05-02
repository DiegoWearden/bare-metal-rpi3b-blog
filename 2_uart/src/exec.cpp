#include "uart.h"
#include "printf.h"
#include "stdint.h"

/**
 * common exception handler
 */

extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    printf("An Exception has ocurred!!!");
           
    // no return from exception for now
    while(1);
}
