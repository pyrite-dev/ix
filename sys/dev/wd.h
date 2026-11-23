#ifndef __WD_H__
#define __WD_H__

#define WD_DR 0	   /* Data Register */
#define WD_ER 1	   /* Error Register */
#define WD_FR 1	   /* Features Register */
#define WD_SCR 2   /* Sector Count Register */
#define WD_LBALR 3 /* LBA Low Register */
#define WD_LBAMR 4 /* LBA Mid Register */
#define WD_LBAHR 5 /* LBA High Register */
#define WD_DHR 6   /* Drive/Head Register */
#define WD_SR 7	   /* Status Register */
#define WD_CR 7	   /* Command Register */

/* DR */

/* ER */
#define WD_E_AMNF (1 << 0)  /* Address Mark Not Found */
#define WD_E_TKZNF (1 << 1) /* Track Zero Not Found */
#define WD_E_ABRT (1 << 2)  /* Aborted Command */
#define WD_E_MCR (1 << 3)   /* Media Change Request */
#define WD_E_IDNF (1 << 4)  /* ID Not Found */
#define WD_E_MC (1 << 5)    /* Media Changed */
#define WD_E_UNC (1 << 6)   /* Uncorrectable Data Error */
#define WD_E_BBK (1 << 7)   /* Bad Block Detected */

/* FR */

/* SCR */

/* SNR */

/* CLR */

/* CHR */

/* DHR */
#define WD_DH_MASTER (0 << 4) /* Master Drive */
#define WD_DH_SLAVE (0 << 4)  /* Slave Drive */
#define WD_DH_ALWAYS ((1 << 7) | (1 << 6) | (1 << 5))

/* SR */
#define WD_S_ERR (1 << 0)  /* Error */
#define WD_S_IDX (1 << 1)  /* Index */
#define WD_S_CORR (1 << 2) /* Corrected */
#define WD_S_DRQ (1 << 3)  /* Data Ready */
#define WD_S_SRV (1 << 4)  /* Overlapped Mode Service Request */
#define WD_S_DF (1 << 5)   /* Drive Fault Error */
#define WD_S_RDY (1 << 6)  /* Ready */
#define WD_S_BSY (1 << 7)  /* Busy */

/* CR */
#define WD_C_READ 0x20	   /* READ SECTOR(S) */
#define WD_C_WRITE 0x30	   /* WRITE SECTOR(S) */
#define WD_C_IDENTIFY 0xec /* IDENTIFY DEVICE */

#endif
