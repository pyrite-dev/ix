#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/errno.h>

int disk_open(struct disk* disk, int flags, int mode) {
	return 0;
}

int disk_close(struct disk* disk, int flags, int mode) {
	return 0;
}

void disk_strategy(struct disk* disk, struct buf* buf) {
	unsigned int   nsec;
	unsigned int   i;
	unsigned char* data;

	buf->error = 0;
	buf->resid = buf->bcount;
	buf->flags &= ~(BUF_DONE | BUF_ERROR);

	mutex_lock(&disk->lock);

	if(buf->bcount == 0) {
		buf->flags |= BUF_DONE;
		mutex_unlock(&disk->lock);
		return;
	}

	if(buf->bcount % 512 != 0 || buf->blkno >= disk->nsectors || buf->bcount / 512 > disk->nsectors - buf->blkno) {
		buf->error = EINVAL;
		buf->flags |= BUF_ERROR | BUF_DONE;
		mutex_unlock(&disk->lock);
		return;
	}

	nsec = buf->bcount / 512;
	data = buf->data;

	for(i = 0; i < nsec; i++) {
		if(disk->ops->transfer(disk, buf->blkno + i, data + i * 512, buf->flags & BUF_READ) != 0) {
			buf->error = EIO;
			buf->flags |= BUF_ERROR;
			break;
		}

		buf->resid -= 512;
	}

	buf->flags |= BUF_DONE;

	mutex_unlock(&disk->lock);
}

int disk_ioctl(struct disk* disk, unsigned long cmd, void* data, int flags) {
	return 0;
}
