#include "framebuffer.h"

#include "dcache.h"
#include "printf.h"
#include "utils.h"
#include "mailbox.h"
#include "atomic.h"
// #include "core.h"

static Framebuffer fb __attribute__((aligned(16)));

static SpinLock fb_lock;

Framebuffer* fb_get(void) {
    return &fb;
}

static constexpr int x_start = 0;
static constexpr int y_start = 0;

static int x = x_start;
static int y = y_start;

static bool fb_init_done = false;

int fb_init(void) {
    printf("Starting framebuffer initialization...\n");

    mailbox[0] = 35 * 4;  // buffer size in bytes
    mailbox[1] = MAILBOX_TAG_REQUEST;

    // set physical width/height (actual size of display)
    mailbox[2] = MAILBOX_TAG_SET_PHYSICAL_SIZE;
    mailbox[3] = 8;
    mailbox[4] = 8;
    mailbox[5] = 640;  // physical width
    mailbox[6] = 480;  // physical height

    // set virtual width/height (where youcan draw)
    mailbox[7] = MAILMBOX_TAG_SET_VIRTUAL_SIZE;
    mailbox[8] = 8;
    mailbox[9] = 8;
    mailbox[10] = 640;  // virtual width
    mailbox[11] = 480;  // virtual height

    // set bit depth
    mailbox[12] = MAILBOX_TAG_SET_DEPTH;
    mailbox[13] = 4;
    mailbox[14] = 4;
    mailbox[15] = 32;  // 32 bits per pixel

    // set pixel order
    mailbox[16] = MAILBOX_TAG_SET_PIXEL_ORDER;
    mailbox[17] = 4;
    mailbox[18] = 4;
    mailbox[19] = 1;  // RGB

    // set framebuffer
    mailbox[20] = MAILBOX_TAG_ALLOCATE_FRAMEBUFFER;
    mailbox[21] = 8;
    mailbox[22] = 8;
    mailbox[23] = 4096;  // framebuffer size
    mailbox[24] = 0;

    // set pitch
    mailbox[25] = MAILBOX_TAG_GET_PITCH;
    mailbox[26] = 4;
    mailbox[27] = 4;
    mailbox[28] = 0;  // pitch

    mailbox[29] = MAILBOX_TAG_END;

    printf("Sending mailbox call...\n");
    if (mailbox_call(MAILBOX_CHANNEL_PROPERTY)) {
        printf("Mailbox call successful\n");
        if (mailbox[23]) {
            printf("Got framebuffer address: %x\n", mailbox[23]);
            mailbox[23] &= 0x3FFFFFFF;
            fb.width = mailbox[10];
            fb.height = mailbox[11];
            fb.pitch = mailbox[28];
            fb.isrgb = mailbox[19];
            fb.buffer = (void*)((unsigned long)mailbox[23]);
            fb.size = mailbox[24];
            fb_init_done = true;
            printf("Framebuffer initialized:\n");
            printf("Width: %d, Height: %d\n", fb.width, fb.height);
            printf("Pitch: %d, Buffer size: %d\n", fb.pitch, fb.size);
            printf("Buffer address: %x\n", (unsigned long)fb.buffer);

            return 1;
        } else {
            printf("Failed to get framebuffer address\n");
        }
    } else {
        printf("Mailbox call failed\n");
    }
    printf("Failed to initialize framebuffer\n");
    return 0;
}

void draw_pixel(int x, int y, unsigned int color) {
    Framebuffer* fb = fb_get();
    if (!fb_init_done || fb->buffer == nullptr || fb->pitch == 0) {
        return; 
    }
    if (x >= 0 && x < (int)fb->width && y >= 0 && y < (int)fb->height) {
        unsigned int* ptr = (unsigned int*)fb->buffer;
        ptr[y * (fb->pitch / 4) + x] = color;
    }
}

void draw_char(int x, int y, char c, unsigned int color) {
    if (!fb_init_done) {
        return;
    }
    
    unsigned int idx = 0;
    if (c >= 32 && c <= 127) {
        idx = c - 32;
    }

    for (int row = 0; row < 8; row++) {
        unsigned char line = font[idx][row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void fb_scroll(int scroll_pixels) {
    if (!fb_init_done) {
        return;
    }
    
    Framebuffer* fb = fb_get();
    
    if (fb->buffer == nullptr || fb->width == 0 || fb->height == 0 || fb->pitch == 0) {
        return;
    }
    
    int row_pixels = fb->pitch / 4;  // number of pixels per row (each pixel is 4 bytes)
    int rows_to_move = fb->height - scroll_pixels;
    unsigned int* buf = (unsigned int*)fb->buffer;

    // Copy each row upward: for each row, copy the row located scroll_pixels below.
    for (int row = 0; row < rows_to_move; row++) {
        for (int col = 0; col < row_pixels; col++) {
            buf[row * row_pixels + col] = buf[(row + scroll_pixels) * row_pixels + col];
        }
    }

    // Clear the bottom scroll_pixels rows (fill with BLACK)
    for (unsigned int row = rows_to_move; row < fb->height; row++) {
        for (int col = 0; col < row_pixels; col++) {
            buf[row * row_pixels + col] = BLACK;
        }
    }
}

// helper to clear one text‐row (8×10 px per char) across the full width
static void clear_cell(int cx, int cy) {
    Framebuffer* fb = fb_get();
    unsigned int* buf   = (unsigned int*)fb->buffer;
    int          pitch  = fb->pitch / 4;
    for (int row = 0; row < 10; row++) {
        for (int col = 0; col < 8;  col++) {
            int px = cx + col, py = cy + row;
            if (px >= 0 && px < (int)fb->width &&
                py >= 0 && py < (int)fb->height) {
                buf[py * pitch + px] = BLACK;
            }
        }
    }
}

void fb_print(const char* str, unsigned int color) {
    fb_lock.lock();
    if (!fb_init_done) { fb_lock.unlock(); return; }
    Framebuffer* fb = fb_get();
    if (!fb->buffer || fb->width==0 || fb->height==0 || fb->pitch==0) {
        fb_lock.unlock(); return;
    }

    while (*str) {
        char c = *str++;
        if (c == '\r') {
            // carriage‑return → back to margin
            x = x_start;
        }
        else if (c == '\n') {
            // newline → scroll or move down, then margin
            if (y + 10 >= (int)fb->height) {
                fb_scroll(10);
                y = fb->height - 10;
            } else {
                y += 10;
            }
            x = x_start;
        }
        else {
            // clear the target cell, then draw the glyph
            clear_cell(x, y);
            draw_char(x, y, c, color);
            x += 8;
            // wrap if past right edge
            if (x + 8 > (int)fb->width) {
                if (y + 10 >= (int)fb->height) {
                    fb_scroll(10);
                    y = fb->height - 10;
                } else {
                    y += 10;
                }
                x = x_start;
            }
        }
    }

    fb_lock.unlock();
}




void fb_clear(unsigned int color) {
    Framebuffer* fb = fb_get();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;

    for (unsigned int y = 0; y < fb->height; y++) {
        for (unsigned int x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = color;
        }
    }
}
