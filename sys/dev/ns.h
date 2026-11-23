#ifndef __NS_H__
#define __NS_H__

#define NS_BUFSZ 256

/* Registers */
#define NS_DR 0	 /* Data Register (DLAB = 0) */
#define NS_DLR 0 /* Divisor Low Register (DLAB = 1) */
#define NS_IER 1 /* Interrupt Enable Register (DLAB = 0) */
#define NS_DHR 1 /* Divisor High Register (DLAB = 1) */
#define NS_IIR 2 /* Interrupt Identification Register */
#define NS_FCR 2 /* FIFO Control Register */
#define NS_LCR 3 /* Line Control Register */
#define NS_MCR 4 /* Modem Control Register */
#define NS_LSR 5 /* Line Status Register */
#define NS_MSR 6 /* Modem Status Register */
#define NS_SCR 7 /* Scratch Register */

/* IER */
#define NS_IE_MS (1 << 3)   /* Modem Status */
#define NS_IE_RLS (1 << 2)  /* Receiver Line Status */
#define NS_IE_THRE (1 << 1) /* Transmitter Holding Register Empty */
#define NS_IE_RDA (1 << 0)  /* Received Data Available */

/* IIR */
#define NS_II_FBS (3 << 6)     /* FIFO Buffer State */
#define NS_II_FBS_N (0 << 6)   /* FIFO Buffer State: None */
#define NS_II_FBS_U (1 << 6)   /* FIFO Buffer State: Unusable */
#define NS_II_FBS_E (2 << 6)   /* FIFO Buffer State: Enabled */
#define NS_II_TIP (1 << 3)     /* Timeout Interrupt Pending (16550) */
#define NS_II_IS (3 << 1)      /* Interrupt State */
#define NS_II_IS_MS (0 << 1)   /* Interrupt State: Modem Status */
#define NS_II_IS_THRE (1 << 1) /* Interrupt State: Transmitter Holding Register Empty */
#define NS_II_IS_RDA (2 << 1)  /* Interrupt State: Received Data Available */
#define NS_II_IS_RLS (3 << 1)  /* Interrupt State: Receiver Line Status */
#define NS_II_IP (1 << 0)      /* Interrupt Pending */

/* FCR */
#define NS_FC_ITL (3 << 6)    /* Interrupt Trigger Level */
#define NS_FC_ITL_1 (0 << 6)  /* Interrupt Trigger Level: 1 Byte */
#define NS_FC_ITL_4 (1 << 6)  /* Interrupt Trigger Level: 4 Byte */
#define NS_FC_ITL_8 (2 << 6)  /* Interrupt Trigger Level: 8 Byte */
#define NS_FC_ITL_14 (3 << 6) /* Interrupt Trigger Level: 14 Byte */
#define NS_FC_DMAMS (1 << 3)  /* DMA Mode Select */
#define NS_FC_CTFIFO (1 << 2) /* Clear Transmit FIFO */
#define NS_FC_CRFIFO (1 << 1) /* Clear Receive FIFO */
#define NS_FC_EFIFO (1 << 0)  /* Enable FIFO */

/* LC */
#define NS_LC_DLAB (1 << 7) /* Divisor Latch Access Bit */
#define NS_LC_BEB (1 << 6)  /* Break Enable Bit */
#define NS_LC_PB (7 << 3)   /* Parity Bits */
#define NS_LC_PB_N (0 << 3) /* Parity Bits: None */
#define NS_LC_PB_O (1 << 3) /* Parity Bits: Odd */
#define NS_LC_PB_E (3 << 3) /* Parity Bits: Even */
#define NS_LC_PB_M (5 << 3) /* Parity Bits: Mark */
#define NS_LC_PB_S (7 << 3) /* Parity Bits: Space */
#define NS_LC_SB (1 << 2)   /* Stop Bits */
#define NS_LC_DB (3 << 0)   /* Data Bits */
#define NS_LC_DB_5 (0 << 0) /* Data Bits: 5 bits */
#define NS_LC_DB_6 (1 << 0) /* Data Bits: 6 bits */
#define NS_LC_DB_7 (2 << 0) /* Data Bits: 7 bits */
#define NS_LC_DB_8 (3 << 0) /* Data Bits: 8 bits */

/* MC */
#define NS_MC_L (1 << 4)   /* Loop */
#define NS_MC_O2 (1 << 3)  /* Out 2 */
#define NS_MC_O1 (1 << 2)  /* Out 1 */
#define NS_MC_RTS (1 << 1) /* Request To Send */
#define NS_MC_DTR (1 << 0) /* Data Terminal Ready */

/* LS */
#define NS_LS_IE (1 << 7)   /* Impending Error */
#define NS_LS_TEMT (1 << 6) /* Transmitter Empty */
#define NS_LS_THRE (1 << 5) /* Transmitter Holding Register Empty */
#define NS_LS_BI (1 << 4)   /* Break Indicator */
#define NS_LS_FE (1 << 3)   /* Framing Error */
#define NS_LS_PE (1 << 2)   /* Parity Error */
#define NS_LS_OE (1 << 1)   /* Overrun Error */
#define NS_LS_DR (1 << 0)   /* Data Ready */

/* MS */
#define NS_MS_DCD (1 << 7)  /* Data Carrier Detect */
#define NS_MS_RI (1 << 6)   /* Ring Indicator */
#define NS_MS_DSR (1 << 5)  /* Data Set Ready */
#define NS_MS_CTS (1 << 4)  /* Clear To Send */
#define NS_MS_DDCD (1 << 3) /* Delta Data Carrier Detect */
#define NS_MS_TERI (1 << 2) /* Trailing Edge of Ring Indicator */
#define NS_MS_DDSR (1 << 1) /* Delta Data Set Ready */
#define NS_MS_DCTS (1 << 0) /* Delta Clear To Send */

#endif
