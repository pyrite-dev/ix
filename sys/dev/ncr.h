#ifndef __NCR_H__
#define __NCR_H__

/* Registers */
#define NCR_DATA 0  /* R: Current Data Bus, W: Output Data */
#define NCR_ICR 1   /* Initiator Command Register */
#define NCR_MODE 2  /* Mode Register */
#define NCR_TCR 3   /* Target Command Register */
#define NCR_CSR 4   /* R: Current SCSI control, W: Select Enable */
#define NCR_BSR 5   /* R: Bus and Status, W: Start DMA Send */
#define NCR_IDATA 6 /* R: Input Data, W: Start DMA Target Receive */
#define NCR_RESET 7 /* R: Reset interrupt, W: Start DMA Initiator Receive */

/* ICR */
#define NCR_ICR_RST (1 << 7)
#define NCR_ICR_ACK (1 << 4)
#define NCR_ICR_BSY (1 << 3)
#define NCR_ICR_SEL (1 << 2)
#define NCR_ICR_ATN (1 << 1)
#define NCR_ICR_DATA (1 << 0)

/* CSR */
#define NCR_CSR_RST (1 << 7)
#define NCR_CSR_BSY (1 << 6)
#define NCR_CSR_REQ (1 << 5)
#define NCR_CSR_MSG (1 << 4)
#define NCR_CSR_CD (1 << 3)
#define NCR_CSR_IO (1 << 2)
#define NCR_CSR_SEL (1 << 1)

/* Phases */
#define NCR_PHASE(csr) ((csr) & (NCR_CSR_MSG | NCR_CSR_CD | NCR_CSR_IO))
#define NCR_PHASE_DATA_OUT 0
#define NCR_PHASE_DATA_IN NCR_CSR_IO
#define NCR_PHASE_COMMAND NCR_CSR_CD
#define NCR_PHASE_STATUS (NCR_CSR_CD | NCR_CSR_IO)
#define NCR_PHASE_MESSAGE_IN (NCR_CSR_MSG | NCR_CSR_CD | NCR_CSR_IO)

#endif
