#ifndef __SYS_UIO_H__
#define __SYS_UIO_H__

struct iovec;
struct uio;

enum uio_seg {
	UIO_USERSPACE,
	UIO_SYSSPACE
};

/*
 * UIO_READ: Kernel -> UIO
 * UIO_WRITE: UIO -> Kernel
 */
enum uio_rw {
	UIO_READ = 0,
	UIO_WRITE
};

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stddef.h>

struct iovec {
	void*  iov_base;
	size_t iov_len;
};

struct uio {
	struct iovec* uio_iov;
	int	      uio_iovcnt;

	off_t  uio_offset;
	size_t uio_resid;

	enum uio_seg uio_segflg;
	enum uio_rw  uio_rw;
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
int uio_init(struct uio* uio, struct iovec* iov, int iovcnt, off_t offset, int rw, int seg);

int uiomove(void* buffer, size_t n, struct uio* uio);
#endif

__END_EXTERN_C__

#endif
