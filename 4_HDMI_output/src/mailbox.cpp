#include "mailbox.h"

#include "dcache.h"
#include "printf.h"
#include "stdint.h"

volatile unsigned int mailbox[35] __attribute__((aligned(16), section(".mailbox")));

void mailbox_write(unsigned char channel, unsigned int value) {
    while (*MAILBOX_STATUS & MAILBOX_FULL) {
    }

    if (channel == MAILBOX_CHANNEL_PROPERTY) {
        unsigned int phys_addr = gpu_convert_address((void*)(uintptr_t)value);
        value = phys_addr;
    }
    *MAILBOX_WRITE = (value & ~0xF) | (channel & 0xF);
}

unsigned int mailbox_read(unsigned char channel) {
    unsigned int response;
    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY);
        response = *MAILBOX_READ;
        if ((response & 0xF) == channel) {
            return response & ~0xF;
        }
    }
}

unsigned int gpu_convert_address(void* addr) {
    unsigned long virtual_addr = (unsigned long)addr;
    unsigned long phys_addr = virtual_addr & ~VA_START;
    unsigned int gpu_addr = (unsigned int)(phys_addr & 0x3FFFFFFF);
    return gpu_addr;
}

unsigned int mailbox_call(unsigned char ch) {
    clean_dcache_range((void*)mailbox, 36 * sizeof(unsigned int));

    unsigned int r = gpu_convert_address((void*)&mailbox[0]);

    mailbox_write(ch, r);
    unsigned int response = mailbox_read(ch);
    invalidate_dcache_range((void*)mailbox, 36 * sizeof(unsigned int));

    return response;
}
