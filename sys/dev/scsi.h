#ifndef __SCSI_H__
#define __SCSI_H__

#include <sys/device.h>

/* Opcodes */
#define SCSI_OP_TEST_UNIT_READY 0x00
#define SCSI_OP_REQUEST_SENSE 0x03
#define SCSI_OP_READ6 0x08
#define SCSI_OP_WRITE6 0x0a
#define SCSI_OP_INQUIRY 0x12
#define SCSI_OP_READ_CAPACITY 0x25
#define SCSI_OP_READ10 0x28
#define SCSI_OP_WRITE10 0x2a

/* Status codes */
#define SCSI_STATUS_GOOD 0x00
#define SCSI_STATUS_CHECK_CONDITION 0x02

struct scsi_adapter {
	int (*exec)(struct device* hba, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read);
};

int scsibus_exec(struct device* bus, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read);

#endif
