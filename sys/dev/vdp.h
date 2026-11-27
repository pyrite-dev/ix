#ifndef __VDP_H__
#define __VDP_H__

#define VDP_DATA 0
#define VDP_ADDR 1

/* Registers */
#define VDP_R0 0x80
#define VDP_R1 0x81
#define VDP_R2 0x82
#define VDP_R3 0x83
#define VDP_R4 0x84
#define VDP_R5 0x85
#define VDP_R6 0x86
#define VDP_R7 0x87

/* Register 0 */
#define VDP_R0_EV (1 << 0)
#define VDP_R0_M3 (1 << 1)

/* Register 1 */
#define VDP_R1_MAG (1 << 0)
#define VDP_R1_SIZE (1 << 1)
#define VDP_R1_M2 (1 << 3)
#define VDP_R1_M1 (1 << 4)
#define VDP_R1_IE (1 << 5)
#define VDP_R1_BLANK (1 << 6)
#define VDP_R1_16K (1 << 7)

#define VDP_COLOR 0xf1

#endif
