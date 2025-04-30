#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "entry.h"
#include "core.h"

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__ ((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

void kernel_init();

extern "C" void print_current_sp() {
    uint64_t core_id, sp;

    // Get core ID from MPIDR_EL1
    asm volatile("mrs %0, mpidr_el1" : "=r"(core_id));
    core_id &= 0xFF;

    // Read current SP
    asm volatile("mov %0, sp" : "=r"(sp));

    printf("Core %lu stack pointer (sp): 0x%lx\n", core_id, sp);
}



extern "C" uint64_t pickKernelStack(void) {
    return (uint64_t) &stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}


extern "C" void secondary_kernel_init(){
    kernel_init();
    
}

extern "C" void primary_kernel_init() {
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        printf("printf initialized!!!\n");
        // Initialize MMU page tables
        // Initialize heap
        smpInitDone = true;
        wake_up_cores();
        kernel_init();
}

void kernel_init(){
    if(getCoreID() == 1)
        print_current_sp();
    // printf("Hi, I'm core %d\n", getCoreID());
}

