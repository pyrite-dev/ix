#ifndef __SYS_BUF_H__
#define __SYS_BUF_H__

struct buf;

#define BUF_WRITE 0x0000
#define BUF_READ 0x0001
#define BUF_DONE 0x0002
#define BUF_ERROR 0x0004

#include <sys/cdefs.h>
#include <sys/types.h>

struct buf {
	dev_t dev;

	unsigned int blkno; /* in 512-byte sectors */
	void*	     data;
	unsigned int bcount;
	unsigned int resid;
	int	     error;
	unsigned int flags;
};

#endif
