#include "vm.h"
#include "libk.h"
#include "stdint.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD_arm[512] __attribute__((aligned(4096), section(".paging")));

/**
 * used for setting up kernel memory for devices
 */
void patch_page_tables() {
    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (~0xFFF);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
    // PUD[1] = PUD[1] & (~0xFFF);
    // PUD[1] = PUD[1] | 0x401;

    for (int i = 0; i < 8; i++) {
        PMD_arm[i] = PMD_arm[i] & (~0xFFF);
        PMD_arm[i] = PMD_arm[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}
