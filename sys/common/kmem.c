#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/spinlock.h>

#include <stdlib.h>
#include <string.h>

typedef union kmem_align kmem_align_t;
typedef union kmem_block kmem_block_t;

union kmem_align {
	size_t a;
	long   b;
	double c;
};

union kmem_block {
	struct {
		size_t	      size;
		kmem_block_t* prev;
		kmem_block_t* next;
	} s;

	kmem_align_t align;
};

#define KMEM_ALIGN (sizeof(kmem_align_t))
#define KMEM_HEADER (sizeof(kmem_block_t))

static kmem_block_t* kmem_free_list = NULL;
static spinlock_t    kmem_spinlock;
static int	     kmem_spinlock_enabled;

vaddr_t kmem_avail;

static size_t kmem_round(size_t size) {
	size_t rem;

	rem = size % KMEM_ALIGN;

	if(rem != 0) size += KMEM_ALIGN - rem;

	return size;
}

static void kmem_remove(kmem_block_t* block) {
	if(block->s.prev == NULL) {
		kmem_free_list = block->s.next;
	} else {
		block->s.prev->s.next = block->s.next;
	}

	if(block->s.next != NULL) block->s.next->s.prev = block->s.prev;

	block->s.next = NULL;
	block->s.prev = NULL;
}

static void kmem_insert(kmem_block_t* block) {
	kmem_block_t* cur;
	kmem_block_t* prev;

	prev = NULL;
	cur  = kmem_free_list;

	while(cur != NULL && (vaddr_t)cur < (vaddr_t)block) {
		prev = cur;
		cur  = cur->s.next;
	}

	block->s.prev = prev;
	block->s.next = cur;

	if(prev == NULL) {
		kmem_free_list = block;
	} else {
		prev->s.next = block;
	}

	if(cur != NULL) cur->s.prev = block;
}

void kmem_init(void* start, void* end) {
	kmem_block_t* block;
	size_t	      size;
	vaddr_t	      s = (vaddr_t)start;
	vaddr_t	      e = (vaddr_t)end;

	s = (s + KMEM_ALIGN - 1) & ~(KMEM_ALIGN - 1);
	e &= ~(KMEM_ALIGN - 1);

	kmem_free_list = NULL;

	size = (size_t)(e - s);

	block	      = (kmem_block_t*)s;
	block->s.size = size;
	block->s.next = NULL;
	block->s.prev = NULL;

	kmem_avail = size;

	kmem_free_list = block;

	libc_malloc  = kmalloc;
	libc_realloc = krealloc;
	libc_free    = kfree;

	spinlock_init(&kmem_spinlock);
	kmem_spinlock_enabled = 0;
}

void kmem_use_spinlock(void) {
	kmem_spinlock_enabled = 1;
}

static intr_flags_t kmem_lock(void) {
	if(kmem_spinlock_enabled) return spinlock_lock_irqsave(&kmem_spinlock);
	return 0;
}

static void kmem_unlock(intr_flags_t flags) {
	if(kmem_spinlock_enabled) spinlock_unlock_irqrestore(&kmem_spinlock, flags);
}

void* kmalloc(size_t size) {
	kmem_block_t* block;
	kmem_block_t* split;
	size_t	      total;
	size_t	      used;
	intr_flags_t  flags;

	if(size == 0) return NULL;

	if(size > (size_t)-1 - KMEM_HEADER - KMEM_ALIGN) return NULL;

	size = kmem_round(size);

	total = KMEM_HEADER + size;

	flags = kmem_lock();
	for(block = kmem_free_list; block != NULL; block = block->s.next) {
		if(block->s.size < total) continue;

		kmem_remove(block);

		used = block->s.size;

		if(block->s.size >= total + KMEM_HEADER + KMEM_ALIGN) {
			split = (kmem_block_t*)((char*)block + total);

			split->s.size = block->s.size - total;
			split->s.next = NULL;
			split->s.prev = NULL;

			block->s.size = total;

			kmem_insert(split);

			used = total;
		}

		kmem_avail -= used;

		kmem_unlock(flags);
		return (void*)(block + 1);
	}
	kmem_unlock(flags);

	return NULL;
}

void* krealloc(void* ptr, size_t size) {
	kmem_block_t* block;
	void*	      newptr;
	size_t	      old_size;
	size_t	      copy_size;

	if(ptr == NULL) return kmalloc(size);

	if(size == 0) {
		kfree(ptr);
		return NULL;
	}

	block = ((kmem_block_t*)ptr) - 1;

	old_size = block->s.size - KMEM_HEADER;

	newptr = kmalloc(size);

	if(newptr == NULL) return NULL;

	copy_size = old_size < size ? old_size : size;

	memcpy(newptr, ptr, copy_size);

	kfree(ptr);

	return newptr;
}

void kfree(void* ptr) {
	kmem_block_t* block;
	kmem_block_t* prev;
	kmem_block_t* next;
	intr_flags_t  flags;

	if(ptr == NULL) return;

	flags = kmem_lock();

	block = ((kmem_block_t*)ptr) - 1;
	kmem_avail += block->s.size;

	kmem_insert(block);

	prev = block->s.prev;
	next = block->s.next;

	if(prev != NULL && (char*)prev + prev->s.size == (char*)block) {
		prev->s.size += block->s.size;
		kmem_remove(block);

		block = prev;
	}

	next = block->s.next;

	if(next != NULL && (char*)block + block->s.size == (char*)next) {
		block->s.size += next->s.size;
		kmem_remove(next);
	}

	kmem_unlock(flags);
}
