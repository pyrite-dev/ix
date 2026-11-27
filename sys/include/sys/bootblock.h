#ifndef __SYS_BOOTBLOCK_H__
#define __SYS_BOOTBLOCK_H__

struct mbr_chs;
struct mbr_partition;
struct mbr_sector;

#define MBR_SIGNATURE 0xaa55

#include <sys/cdefs.h>

struct mbr_chs {
	unsigned char head;
	unsigned char cyl_sector;
	unsigned char sector;
} __packed;

struct mbr_partition {
	unsigned char  flag;
	struct mbr_chs start;
	unsigned char  type;
	struct mbr_chs end;
	unsigned int   start_lba;
	unsigned int   sectors;
} __packed;

struct mbr_sector {
	unsigned char	     code[446];
	struct mbr_partition table[4];
	unsigned short	     sig;
} __packed;

#endif
