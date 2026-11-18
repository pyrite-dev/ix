#include <sys/uio.h>
#include <sys/types.h>
#include <sys/uaccess.h>
#include <sys/errno.h>

#include <string.h>

int uio_init(struct uio* uio, struct iovec* iov, int iovcnt, off_t offset, int rw, int seg) {
	size_t resid;
	int    i;

	if(uio == NULL) return EINVAL;
	if(iovcnt < 0) return EINVAL;
	if(iovcnt > 0 && iov == NULL) return EINVAL;

	if(rw != UIO_READ && rw != UIO_WRITE) return EINVAL;

	if(seg != UIO_USERSPACE && seg != UIO_SYSSPACE) return EINVAL;

	resid = 0;

	for(i = 0; i < iovcnt; i++) {
		if(iov[i].iov_len > (size_t)-1 - resid) return EINVAL; /* TODO: replace this with overflow */
		resid += iov[i].iov_len;
	}

	uio->uio_iov	= iov;
	uio->uio_iovcnt = iovcnt;
	uio->uio_offset = offset;
	uio->uio_resid	= resid;
	uio->uio_rw	= rw;
	uio->uio_segflg = seg;

	return 0;
}

static void uio_skip_empty(struct uio* uio) {
	while(uio->uio_iovcnt > 0 && uio->uio_iov->iov_len == 0) {
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}
}

int uiomove(void* buffer, size_t n, struct uio* uio) {
	unsigned char* kaddr;
	unsigned char* uaddr;
	size_t	       count;
	int	       error;

	if(n == 0 || uio->uio_resid == 0) return 0;

	if(n > uio->uio_resid) n = uio->uio_resid;

	kaddr = (unsigned char*)buffer;

	while(n > 0) {
		uio_skip_empty(uio);

		uaddr = (unsigned char*)uio->uio_iov->iov_base;

		count = uio->uio_iov->iov_len;

		if(count > n) count = n;

		if(uio->uio_rw == UIO_READ) {
			if(uio->uio_segflg == UIO_USERSPACE) {
				error = copyout(uaddr, kaddr, count);
			} else {
				memmove(uaddr, kaddr, count);
				error = 0;
			}
		} else {
			if(uio->uio_segflg == UIO_USERSPACE) {
				error = copyin(kaddr, uaddr, count);
			} else {
				memmove(kaddr, uaddr, count);
				error = 0;
			}
		}

		if(error != 0) return error;

		uio->uio_iov->iov_base = (unsigned char*)uio->uio_iov->iov_base + count;

		uio->uio_iov->iov_len -= count;

		uio->uio_offset += (off_t)count;
		uio->uio_resid -= count;

		kaddr += count;
		n -= count;
	}

	return 0;
}
