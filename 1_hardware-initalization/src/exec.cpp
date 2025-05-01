#include "uart.h"
#include "printf.h"
#include "stdint.h"

/**
 * common exception handler
 */

extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    //will branch here in the case of an en exception
           
    // no return from exception for now
    while(1);
}
