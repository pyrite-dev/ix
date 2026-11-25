#include <sys/file.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/major.h>
#include <sys/thread.h>
#include <sys/uaccess.h>
#include <sys/uio.h>

#include <string.h>
#include <stdlib.h>

#define FILE_BLOCK_SIZE 512

struct file {
	dev_t	     dev;
	int	     flags;
	int	     is_block;
	unsigned int ref;
	off_t	     offset;

	off_t	      cache_blkno;
	int	      cache_valid;
	int	      cache_dirty;
	unsigned char cache[FILE_BLOCK_SIZE];
};

static struct file* file_open_device(dev_t dev, int is_block, int flags, int* error);

static dev_t file_std_dev;
static int   file_std_valid;

static void file_bind_std(thread_t* thread) {
	int fd;

	thread->files_inited = 1;

	if(!file_std_valid) return;

	for(fd = 0; fd < 3; fd++) {
		int error;

		thread->files[fd] = file_open_device(file_std_dev, 0, O_RDWR, &error);
	}
}

static thread_t* file_current(void) {
	thread_t* thread = thread_current();

	if(thread == NULL) return NULL;

	if(!thread->files_inited) file_bind_std(thread);

	return thread;
}

static struct file* file_get(int fd) {
	thread_t*    thread = file_current();
	struct file* file;

	if(thread == NULL) return NULL;
	if(fd < 0 || fd >= NFILE) return NULL;

	file = thread->files[fd];
	if(file == NULL) return NULL;

	file->ref++;

	return file;
}

static void file_put(struct file* file) {
	file->ref--;
}

static int file_block_io(struct file* file, int read) {
	struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(file->dev));
	struct buf     b;

	memset(&b, 0, sizeof(b));
	b.dev	 = file->dev;
	b.blkno	 = file->cache_blkno;
	b.data	 = file->cache;
	b.bcount = FILE_BLOCK_SIZE;
	b.flags	 = read ? BUF_READ : BUF_WRITE;

	sw->strategy(&b);

	if(b.flags & BUF_ERROR) return b.error != 0 ? b.error : EIO;

	return 0;
}

static int file_block_flush(struct file* file) {
	int error;

	if(!file->cache_valid || !file->cache_dirty) return 0;

	error = file_block_io(file, 0);
	if(error != 0) return error;

	file->cache_dirty = 0;

	return 0;
}

static int file_block_fill(struct file* file, off_t blkno) {
	int error;

	if(file->cache_valid && file->cache_blkno == blkno) return 0;

	error = file_block_flush(file);
	if(error != 0) return error;

	file->cache_blkno = blkno;
	file->cache_valid = 0;

	error = file_block_io(file, 1);
	if(error != 0) return error;

	file->cache_valid = 1;

	return 0;
}

static void file_release(struct file* file) {
	if(file->ref != 0) return;

	if(file->is_block) {
		struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(file->dev));

		file_block_flush(file);

		if(sw != NULL && sw->close != NULL) sw->close(file->dev, file->flags, 0);
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw != NULL && sw->close != NULL) sw->close(file->dev, file->flags, 0);
	}

	free(file);
}

static int file_fdalloc(struct file* file) {
	thread_t* thread = file_current();
	int	  fd;

	if(thread == NULL) return -EMFILE;

	for(fd = 0; fd < NFILE; fd++) {
		if(thread->files[fd] == NULL) {
			thread->files[fd] = file;
			return fd;
		}
	}

	return -EMFILE;
}

static struct file* file_open_device(dev_t dev, int is_block, int flags, int* error) {
	struct file* file;
	int	     err;

	file = (struct file*)malloc(sizeof(struct file));
	if(file == NULL) {
		*error = ENOMEM;
		return NULL;
	}

	memset(file, 0, sizeof(struct file));
	file->dev      = dev;
	file->flags    = flags;
	file->is_block = is_block;
	file->ref      = 1;

	if(is_block) {
		struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(dev));

		err = (sw != NULL && sw->open != NULL) ? sw->open(dev, flags, 0) : ENXIO;
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(dev));

		err = (sw != NULL && sw->open != NULL) ? sw->open(dev, flags, 0) : ENXIO;
	}

	if(err != 0) {
		free(file);
		*error = err;
		return NULL;
	}

	return file;
}

void file_init(void) {
	file_std_valid = cdev_first_device(CDEV_MAJOR_TTY, &file_std_dev) == 0;
}

int sys_open(const char* path, int flags) {
	char	     name[128];
	dev_t	     dev;
	struct file* file;
	int	     error;
	int	     fd;

	if((flags & 3) == 3) return -EINVAL;

	error = copyinstr(name, path, sizeof(name), NULL);
	if(error != 0) return -error;

	if(name[0] != '/' || name[1] != 'd' || name[2] != 'e' || name[3] != 'v' || name[4] != '/') return -ENOENT;

	if(cdev_find_device(name + 5, &dev) == 0) {
		file = file_open_device(dev, 0, flags, &error);
	} else if(bdev_find_device(name + 5, &dev) == 0) {
		file = file_open_device(dev, 1, flags, &error);
	} else {
		return -ENOENT;
	}

	if(file == NULL) return -error;

	fd = file_fdalloc(file);
	if(fd < 0) {
		file->ref = 0;
		file_release(file);
		return fd;
	}

	return fd;
}

int sys_close(int fd) {
	thread_t*    thread = file_current();
	struct file* file;

	if(thread == NULL) return -EBADF;
	if(fd < 0 || fd >= NFILE) return -EBADF;

	file = thread->files[fd];
	if(file == NULL) return -EBADF;

	thread->files[fd] = NULL;

	file_put(file);
	file_release(file);

	return 0;
}

int sys_read(int fd, void* buffer, size_t count) {
	struct file* file = file_get(fd);
	int	     result;

	if(file == NULL) return -EBADF;

	if((file->flags & 3) == O_WRONLY) {
		file_put(file);
		return -EBADF;
	}

	if(file->is_block) {
		struct iovec iov;
		struct uio   uio;
		int	     error;

		iov.iov_base = buffer;
		iov.iov_len  = count;

		error = uio_init(&uio, &iov, 1, file->offset, UIO_READ, UIO_USERSPACE);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		while(uio.uio_resid > 0) {
			off_t  blkno = uio.uio_offset / FILE_BLOCK_SIZE;
			size_t off   = (size_t)(uio.uio_offset % FILE_BLOCK_SIZE);
			size_t n     = FILE_BLOCK_SIZE - off;

			if(n > uio.uio_resid) n = uio.uio_resid;

			error = file_block_fill(file, blkno);
			if(error != 0) break;

			error = uiomove(file->cache + off, n, &uio);
			if(error != 0) break;
		}

		file->offset = uio.uio_offset;
		result	     = (int)(count - uio.uio_resid);

		if(result == 0 && error != 0) result = -error;
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));
		struct iovec   iov;
		struct uio     uio;
		int	       error;

		if(sw == NULL || sw->read == NULL) {
			file_put(file);
			return -ENXIO;
		}

		iov.iov_base = buffer;
		iov.iov_len  = count;

		error = uio_init(&uio, &iov, 1, file->offset, UIO_READ, UIO_USERSPACE);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		error = sw->read(file->dev, &uio, file->flags & O_NONBLOCK);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		result = (int)(count - uio.uio_resid);
	}

	file_put(file);

	return result;
}

int sys_write(int fd, const void* buffer, size_t count) {
	struct file* file = file_get(fd);
	int	     result;

	if(file == NULL) return -EBADF;

	if((file->flags & 3) == O_RDONLY) {
		file_put(file);
		return -EBADF;
	}

	if(file->is_block) {
		struct iovec iov;
		struct uio   uio;
		int	     error;

		iov.iov_base = (void*)buffer;
		iov.iov_len  = count;

		error = uio_init(&uio, &iov, 1, file->offset, UIO_WRITE, UIO_USERSPACE);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		while(uio.uio_resid > 0) {
			off_t  blkno = uio.uio_offset / FILE_BLOCK_SIZE;
			size_t off   = (size_t)(uio.uio_offset % FILE_BLOCK_SIZE);
			size_t n     = FILE_BLOCK_SIZE - off;
			size_t before;

			if(n > uio.uio_resid) n = uio.uio_resid;

			error = file_block_fill(file, blkno);
			if(error != 0) break;

			before = uio.uio_resid;

			error = uiomove(file->cache + off, n, &uio);

			if(uio.uio_resid < before) file->cache_dirty = 1;

			if(error != 0) break;
		}

		file->offset = uio.uio_offset;
		result	     = (int)(count - uio.uio_resid);

		if(result == 0 && error != 0) result = -error;
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));
		struct iovec   iov;
		struct uio     uio;
		int	       error;

		if(sw == NULL || sw->write == NULL) {
			file_put(file);
			return -ENXIO;
		}

		iov.iov_base = (void*)buffer;
		iov.iov_len  = count;

		error = uio_init(&uio, &iov, 1, file->offset, UIO_WRITE, UIO_USERSPACE);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		error = sw->write(file->dev, &uio, file->flags & O_NONBLOCK);
		if(error != 0) {
			file_put(file);
			return -error;
		}

		result = (int)(count - uio.uio_resid);
	}

	file_put(file);

	return result;
}
