#ifndef __PIT_H__
#define __PIT_H__

/* Registers */
#define PIT_CH0 0
#define PIT_CH1 1
#define PIT_CH2 2
#define PIT_MC 3

/* Mode/Command */
#define PIT_MC_CH (3 << 6)
#define PIT_MC_CH0 (0 << 6)
#define PIT_MC_CH1 (1 << 6)
#define PIT_MC_CH2 (2 << 6)
#define PIT_MC_RB (3 << 6)

#define PIT_MC_AM (3 << 4)
#define PIT_MC_LATCH (0 << 4)
#define PIT_MC_LO (1 << 4)
#define PIT_MC_HI (2 << 4)
#define PIT_MC_LOHI (3 << 4)

#define PIT_MC_MODE (7 << 1)
#define PIT_MC_MODE0 (0 << 1)
#define PIT_MC_MODE1 (1 << 1)
#define PIT_MC_MODE2 (2 << 1)
#define PIT_MC_MODE3 (3 << 1)
#define PIT_MC_MODE4 (4 << 1)
#define PIT_MC_MODE5 (5 << 1)

#define PIT_MC_BIN (0 << 0)
#define PIT_MC_BCD (1 << 0)

#endif
