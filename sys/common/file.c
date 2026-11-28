#include <sys/file.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/major.h>
#include <sys/thread.h>
#include <sys/uaccess.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/dirent.h>
#include <sys/unistd.h>

#include <string.h>
#include <stdlib.h>

#define FILE_BLOCK_SIZE 512

struct file {
	dev_t	     dev;
	int	     flags;
	int	     is_block;
	unsigned int ref;
	off_t	     offset;

	struct vnode* vnode;

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

	if(file->vnode != NULL && file->vnode->type == VNODE_DIR) {
		/* nothing to close */
	} else if(file->is_block) {
		struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(file->dev));

		file_block_flush(file);

		if(sw != NULL && sw->close != NULL) sw->close(file->dev, file->flags, 0);
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw != NULL && sw->close != NULL) sw->close(file->dev, file->flags, 0);
	}

	if(file->vnode != NULL) vnode_put(file->vnode);

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

	file = malloc(sizeof(*file));
	if(file == NULL) {
		*error = ENOMEM;
		return NULL;
	}

	memset(file, 0, sizeof(*file));
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

static struct file* file_open_vnode(struct vnode* vnode, int flags, int* error) {
	struct file* file;

	if(vnode->type == VNODE_DIR) {
		file = malloc(sizeof(*file));
		if(file == NULL) {
			*error = ENOMEM;
			return NULL;
		}

		memset(file, 0, sizeof(*file));
		file->flags = flags;
		file->ref   = 1;
		file->vnode = vnode;

		return file;
	}

	if(vnode->type != VNODE_CHR && vnode->type != VNODE_BLK) {
		*error = ENXIO;
		return NULL;
	}

	file = file_open_device(vnode->data.dev, vnode->type == VNODE_BLK, flags, error);
	if(file == NULL) return NULL;

	file->vnode = vnode;

	return file;
}

int sys_open(const char* path, int flags) {
	char	      name[128];
	struct vnode* vnode;
	struct file*  file;
	int	      error;
	int	      fd;

	if((flags & 3) == 3) return -EINVAL;

	error = copyinstr(name, path, sizeof(name), NULL);
	if(error != 0) return -error;

	error = vfs_lookup(name, &vnode);
	if(error != 0) return -error;

	file = file_open_vnode(vnode, flags, &error);
	if(file == NULL) {
		vnode_put(vnode);
		return -error;
	}

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

	if(file->vnode != NULL && file->vnode->type == VNODE_DIR) {
		file_put(file);
		return -EISDIR;
	}

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

		file->offset = uio.uio_offset;
		result	     = (int)(count - uio.uio_resid);
	}

	file_put(file);

	return result;
}

int sys_write(int fd, const void* buffer, size_t count) {
	struct file* file = file_get(fd);
	int	     result;

	if(file == NULL) return -EBADF;

	if(file->vnode != NULL && file->vnode->type == VNODE_DIR) {
		file_put(file);
		return -EISDIR;
	}

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

		file->offset = uio.uio_offset;
		result	     = (int)(count - uio.uio_resid);
	}

	file_put(file);

	return result;
}

static int file_device_size(struct file* file, off_t* size) {
	if(file->is_block) {
		struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw == NULL || sw->ioctl == NULL) return ENOTTY;

		return sw->ioctl(file->dev, DIOCGSIZE, size, file->flags);
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw == NULL || sw->ioctl == NULL) return ENOTTY;

		return sw->ioctl(file->dev, DIOCGSIZE, size, file->flags);
	}
}

int sys_lseek(int fd, off_t offset, int whence) {
	struct file* file = file_get(fd);
	off_t	     size;
	off_t	     new_offset;

	if(file == NULL) return -EBADF;

	if(file->vnode != NULL && file->vnode->type == VNODE_DIR) {
		file_put(file);
		return -EISDIR;
	}

	if(file_device_size(file, &size) != 0) {
		file_put(file);
		return -ESPIPE;
	}

	switch(whence) {
	case SEEK_SET:
		new_offset = offset;
		break;
	case SEEK_CUR:
		new_offset = file->offset + offset;
		break;
	case SEEK_END:
		new_offset = size + offset;
		break;
	default:
		file_put(file);
		return -EINVAL;
	}

	if(new_offset < 0) {
		file_put(file);
		return -EINVAL;
	}

	file->offset = new_offset;

	file_put(file);

	return new_offset;
}

int sys_ioctl(int fd, unsigned long cmd, void* data) {
	struct file*  file = file_get(fd);
	unsigned long dir;
	size_t	      plen;
	char	      buffer[128];
	char*	      kbuffer = buffer;
	void*	      kdata   = data;
	int	      error   = 0;

	if(file == NULL) return -EBADF;

	if(file->vnode != NULL && file->vnode->type == VNODE_DIR) {
		file_put(file);
		return -ENOTTY;
	}

	dir  = IOC_DIR(cmd);
	plen = IOC_PLEN(cmd);

	if(dir != IOC_VOID && plen > 0) {
		if(plen > sizeof(buffer)) {
			kbuffer = malloc(plen);
			if(kbuffer == NULL) {
				file_put(file);
				return -ENOMEM;
			}
		}

		kdata = kbuffer;

		if((dir & IOC_IN) != 0) {
			error = copyin(kdata, data, plen);
			if(error != 0) goto out;
		} else {
			memset(kdata, 0, plen);
		}
	}

	if(file->is_block) {
		struct bdevsw* sw = bdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw == NULL || sw->ioctl == NULL) {
			error = ENOTTY;
		} else {
			error = sw->ioctl(file->dev, cmd, kdata, file->flags);
		}
	} else {
		struct cdevsw* sw = cdev_lookup(DEVICE_MAJOR(file->dev));

		if(sw == NULL || sw->ioctl == NULL) {
			error = ENOTTY;
		} else {
			error = sw->ioctl(file->dev, cmd, kdata, file->flags);
		}
	}

	if(error == 0 && (dir & IOC_OUT) != 0 && kdata != data) error = copyout(data, kdata, plen);

out:
	if(kbuffer != buffer) free(kbuffer);

	file_put(file);

	return -error;
}

int sys_getdents(int fd, void* buffer, size_t count) {
	struct file*  file = file_get(fd);
	struct dirent entry;
	size_t	      done  = 0;
	int	      error = 0;

	if(file == NULL) return -EBADF;

	if(file->vnode == NULL || file->vnode->type != VNODE_DIR || file->vnode->ops == NULL || file->vnode->ops->readdir == NULL) {
		file_put(file);
		return -ENOTDIR;
	}

	while(done + sizeof(entry) <= count) {
		memset(&entry, 0, sizeof(entry));

		error = file->vnode->ops->readdir(file->vnode, file->offset, &entry.d_ino, entry.d_name, sizeof(entry.d_name));
		if(error != 0) break;

		error = copyout((char*)buffer + done, &entry, sizeof(entry));
		if(error != 0) break;

		done += sizeof(entry);
		file->offset++;
	}

	file_put(file);

	if(done > 0) return (int)done;

	if(error == ENOENT) return 0;

	if(error != 0) return -error;

	return -EINVAL;
}
