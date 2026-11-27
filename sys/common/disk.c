#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include <string.h>

void disk_init(struct disk* disk, const struct disk_ops* ops) {
	memset(disk, 0, sizeof(*disk));

	disk->ops = ops;
}

int disk_open(struct disk* disk, int flags, int mode) {
	return 0;
}

int disk_close(struct disk* disk, int flags, int mode) {
	return 0;
}

void disk_strategy(struct disk* disk, struct buf* buf) {
	struct disk*   phys = disk->parent != NULL ? disk->parent : disk;
	unsigned int   nsec;
	unsigned int   i;
	unsigned char* data;

	buf->error = 0;
	buf->resid = buf->bcount;
	buf->flags &= ~(BUF_DONE | BUF_ERROR);

	mutex_lock(&phys->lock);

	if(buf->bcount == 0) {
		buf->flags |= BUF_DONE;
		mutex_unlock(&phys->lock);
		return;
	}

	if(buf->bcount % 512 != 0 || buf->blkno >= disk->nsectors || buf->bcount / 512 > disk->nsectors - buf->blkno) {
		buf->error = EINVAL;
		buf->flags |= BUF_ERROR | BUF_DONE;
		mutex_unlock(&phys->lock);
		return;
	}

	nsec = buf->bcount / 512;
	data = buf->data;

	for(i = 0; i < nsec; i++) {
		if(phys->ops->transfer(phys, disk->start + buf->blkno + i, data + i * 512, buf->flags & BUF_READ) != 0) {
			buf->error = EIO;
			buf->flags |= BUF_ERROR;
			break;
		}

		buf->resid -= 512;
	}

	buf->flags |= BUF_DONE;

	mutex_unlock(&phys->lock);
}

int disk_ioctl(struct disk* disk, unsigned long cmd, void* data, int flags) {
	return 0;
}
