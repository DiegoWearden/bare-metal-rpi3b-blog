#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "peripherals/base.h"
#include "printf.h"
#include "vm.h"

// Define mailbox base address (for example, on the Pi3)
#define MMIO_BASE (VA_START | 0x3F000000)
#define MAILBOX_BASE (MMIO_BASE + 0xB880)

// Verify alignment of mailbox registers
static_assert((MAILBOX_BASE & 0xF) == 0, "Mailbox base must be 16-byte aligned");

// Define mailbox registers (offsets from MAILBOX_BASE)
#define MAILBOX_READ ((volatile unsigned int*)(MAILBOX_BASE + 0x0))
#define MAILBOX_STATUS ((volatile unsigned int*)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE ((volatile unsigned int*)(MAILBOX_BASE + 0x20))

// Mailbox status flags
#define MAILBOX_FULL 0x80000000   // Write register is full
#define MAILBOX_EMPTY 0x40000000  // Read register is empty

// Mailbox Channels — Used for communication between ARM and VideoCore
#define MAILBOX_CHANNEL_POWER        0  // Power management
#define MAILBOX_CHANNEL_FRAMEBUFFER  1  // Framebuffer configuration
#define MAILBOX_CHANNEL_VIRTUAL_UART 2  // Virtual UART (deprecated, rarely used)
#define MAILBOX_CHANNEL_VCHIQ        3  // VCHIQ interface (used by firmware)
#define MAILBOX_CHANNEL_LEDS         4  // LED control
#define MAILBOX_CHANNEL_BUTTONS      5  // Button input
#define MAILBOX_CHANNEL_TOUCHSCREEN  6  // Touchscreen input
#define MAILBOX_CHANNEL_COUNT        7  // Number of channels (not a real channel)
#define MAILBOX_CHANNEL_PROPERTY     8  // Property channel (used for queries and configuration)


// Mailbox Tags — Used for structured communication over the Property channel
#define MAILBOX_TAG_REQUEST          0  // Request code
#define MAILBOX_TAG_END              0  // End of tags


// VideoCore Property Tags — Sent via the Property channel to configure framebuffer
#define MAILBOX_TAG_SET_PHYSICAL_SIZE   0x48003  // Set physical width/height of framebuffer
#define MAILMBOX_TAG_SET_VIRTUAL_SIZE    0x48004  // Set virtual width/height (scrolling, double buffering)
#define MAILBOX_TAG_SET_DEPTH           0x48005  // Set bits per pixel
#define MAILBOX_TAG_SET_PIXEL_ORDER     0x48006  // Set RGB/BGR pixel order
#define MAILBOX_TAG_ALLOCATE_FRAMEBUFFER 0x40001 // Get framebuffer pointer
#define MAILBOX_TAG_GET_PITCH           0x40008  // Get framebuffer pitch (bytes per row)


// Global mailbox buffer
extern volatile unsigned int mailbox[35] __attribute__((aligned(16), section(".mailbox")));

#ifdef __cplusplus
extern "C" {
#endif

extern void clear_mailbox(void);
extern void mailbox_write(unsigned char channel, unsigned int value);
extern unsigned int mailbox_read(unsigned char channel);
extern unsigned int mailbox_call(unsigned char ch);
extern unsigned int gpu_convert_address(void* addr);

#ifdef __cplusplus
}
#endif

#endif  // _MAILBOX_H
