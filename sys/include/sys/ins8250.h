#ifndef __SYS_INS8250_H__
#define __SYS_INS8250_H__

/* Registers */
#define DR		0		/* Data Register			(DLAB = 0) */
#define DLR		0		/* Divisor Low Register			(DLAB = 1) */
#define IER		1		/* Interrupt Enable Register		(DLAB = 0) */
#define DHR		1		/* Divisor High Register		(DLAB = 1) */
#define IIR		2		/* Interrupt Identification Register	*/
#define FCR		2		/* FIFO Control Register		*/
#define LCR		3		/* Line Control Register		*/
#define MCR		4		/* Modem Control Register		*/
#define LSR		5		/* Line Status Register			*/
#define MSR		6		/* Modem Status Register		*/
#define SCR		7		/* Scratch Register			*/

/* IER */
#define IE_MS		(1 << 3)	/* Modem Status				*/
#define IE_RLS		(1 << 2)	/* Receiver Line Status			*/
#define IE_THRE		(1 << 1)	/* Transmitter Holding Register Empty	*/
#define IE_RDA		(1 << 0)	/* Received Data Available		*/

/* IIR */
#define II_FBS		(3 << 6)	/* FIFO Buffer State					*/
#define II_FBS_N	(0 << 6)	/* FIFO Buffer State: None				*/
#define II_FBS_U	(1 << 6)	/* FIFO Buffer State: Unusable				*/
#define II_FBS_E	(2 << 6)	/* FIFO Buffer State: Enabled				*/
#define II_TIP		(1 << 3)	/* Timeout Interrupt Pending (16550)			*/
#define II_IS		(3 << 3)	/* Interrupt State					*/
#define II_IS_MS	(0 << 3)	/* Interrupt State: Modem Status			*/
#define II_IS_THRE	(1 << 3)	/* Interrupt State: Transmitter Holding Register Empty	*/
#define II_IS_RDA	(2 << 3)	/* Interrupt State: Received Data Available		*/
#define II_IS_RLS	(3 << 3)	/* Interrupt State: Receiver Line Status		*/
#define II_IP		(1 << 0)	/* Interrupt Pending					*/

/* FCR */
#define FC_ITL		(3 << 6)	/* Interrupt Trigger Level		*/
#define FC_ITL_1	(0 << 6)	/* Interrupt Trigger Level: 1 Byte	*/
#define FC_ITL_4	(1 << 6)	/* Interrupt Trigger Level: 4 Byte	*/
#define FC_ITL_8	(2 << 6)	/* Interrupt Trigger Level: 8 Byte	*/
#define FC_ITL_14	(3 << 6)	/* Interrupt Trigger Level: 14 Byte	*/
#define FC_DMAMS	(1 << 3)	/* DMA Mode Select			*/
#define FC_CTFIFO	(1 << 2)	/* Clear Transmit FIFO			*/
#define FC_CRFIFO	(1 << 1)	/* Clear Receive FIFO			*/
#define FC_EFIFO	(1 << 0)	/* Enable FIFO				*/

/* LC */
#define LC_DLAB		(1 << 7)	/* Divisor Latch Access Bit	*/
#define LC_BEB		(1 << 6)	/* Break Enable Bit		*/
#define LC_PB		(7 << 3)	/* Parity Bits			*/
#define LC_PB_N		(0 << 3)	/* Parity Bits: None		*/
#define LC_PB_O		(1 << 3)	/* Parity Bits: Odd		*/
#define LC_PB_E		(3 << 3)	/* Parity Bits: Even		*/
#define LC_PB_M		(5 << 3)	/* Parity Bits: Mark		*/
#define LC_PB_S		(7 << 3)	/* Parity Bits: Space		*/
#define LC_SB		(1 << 2)	/* Stop Bits			*/
#define LC_DB		(3 << 0)	/* Data Bits			*/
#define LC_DB_5		(0 << 0)	/* Data Bits: 5 bits		*/
#define LC_DB_6		(1 << 0)	/* Data Bits: 6 bits		*/
#define LC_DB_7		(2 << 0)	/* Data Bits: 7 bits		*/
#define LC_DB_8		(3 << 0)	/* Data Bits: 8 bits		*/

/* MC */
#define MC_L		(1 << 4)	/* Loop			*/
#define MC_O2		(1 << 3)	/* Out 2		*/
#define MC_O1		(1 << 2)	/* Out 1		*/
#define MC_RTS		(1 << 1)	/* Request To Send	*/
#define MC_DTR		(1 << 0)	/* Data Terminal Ready	*/

/* LS */
#define LS_IE		(1 << 7)	/* Impending Error			*/
#define LS_TEMT		(1 << 6)	/* Transmitter Empty			*/
#define LS_THRE		(1 << 5)	/* Transmitter Holding Register Empty	*/
#define LS_BI		(1 << 4)	/* Break Indicator			*/
#define LS_FE		(1 << 3)	/* Framing Error			*/
#define LS_PE		(1 << 2)	/* Parity Error				*/
#define LS_OE		(1 << 1)	/* Overrun Error			*/
#define LS_DR		(1 << 0)	/* Data Ready				*/

/* MS */
#define MS_DCD		(1 << 7)	/* Data Carrier Detect			*/
#define MS_RI		(1 << 6)	/* Ring Indicator			*/
#define MS_DSR		(1 << 5)	/* Data Set Ready			*/
#define MS_CTS		(1 << 4)	/* Clear To Send			*/
#define MS_DDCD		(1 << 3)	/* Delta Data Carrier Detect		*/
#define MS_TERI		(1 << 2)	/* Trailing Edge of Ring Indicator	*/
#define MS_DDSR		(1 << 1)	/* Delta Data Set Ready			*/
#define MS_DCTS		(1 << 0)	/* Delta Clear To Send			*/

#endif
