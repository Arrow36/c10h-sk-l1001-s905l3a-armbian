// SPDX-License-Identifier: MIT
/*
 * C10H / SK-L1001 Display Hardware Register Direct MMIO Fix Tool
 *
 * This tool directly programs Amlogic S905L3A VPU and MIPI-DSI registers via /dev/mem
 * using experimental values associated with improved output in C10H test records.
 * These values are specific to the tested hardware and mode, not a compatibility guarantee.
 *
 * Experimental settings (individual effects require controlled validation):
 * 1. Color Shift (Green black, magenta white): VPU[0x38A0] = 0x0001C040 (RGB -> YUV709l HDR2 matrix)
 * 2. Half-Purple Screen / Folding: VPU[0x1DF1] = 0x05000320 (OSD1 input size 800x1280)
 * 3. High-Frequency Vertical Jitter: VPU[0x1A2B] = 0x82820501 (VIU FIFO burst 96, hold 8 lines)
 * 4. VPP Output FIFO Configuration: VPU[0x1DF5] = 0x0000077F (Full 1919-depth FIFO)
 * 5. VPP Hold Lines: VPU[0x1D22] = 0x08080808 (8 lines hold)
 * 6. DSI Synchronization & Polarities: DSI[0x3C8]=0x00512430, DSI[0x014]=0x0, DSI[0x050]=0x000002FF
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define VPU_BASE_PHYS   0xff900000
#define VPU_MAP_SIZE    0x40000

#define DSI_BASE_PHYS   0xffd07000
#define DSI_MAP_SIZE    0x1000

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Failed to open /dev/mem (root required)");
        return 1;
    }

    volatile uint32_t *vpu = mmap(NULL, VPU_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, VPU_BASE_PHYS);
    volatile uint32_t *dsi = mmap(NULL, DSI_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DSI_BASE_PHYS);

    if (vpu && vpu != MAP_FAILED) {
        /* 1. Color fix: OSD1 HDR2 RGB -> YUV709l matrix */
        vpu[0x38a0] = 0x0001c040;

        /* 2. Blend size match: VPP OSD1 input size = 800x1280 (no purple blocks/folding) */
        vpu[0x1df1] = 0x05000320;

        /* 3. Jitter/twitch fix: VIU OSD1 FIFO ctrl = 0x82820501 (burst 96, hold 8 lines) */
        vpu[0x1a2b] = 0x82820501;

        /* 4. Jitter/twitch fix: VPP Output FIFO size = 0x77f (stock full FIFO) */
        vpu[0x1df5] = 0x0000077f;

        /* 5. VPP hold lines = 8 lines */
        vpu[0x1d22] = 0x08080808;

        munmap((void *)vpu, VPU_MAP_SIZE);
    } else {
        perror("mmap VPU failed");
    }

    if (dsi && dsi != MAP_FAILED) {
        /* 6. DSI TOP sync polarity: invert HS/VS to active-high (JD9366TC match) */
        dsi[0x3c8 / 4] = 0x00512430;

        /* 7. DSI DPI core polarity: active-high */
        dsi[0x014 / 4] = 0x00000000;

        /* 8. DSI Horizontal line time: 767 lane-byte clocks (matches 531/7 MHz) */
        dsi[0x050 / 4] = 0x000002ff;

        munmap((void *)dsi, DSI_MAP_SIZE);
    } else {
        perror("mmap DSI failed");
    }

    close(fd);
    return 0;
}
