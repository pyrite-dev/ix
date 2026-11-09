#include <sys/kmem.h>
#include <string.h>

typedef uintptr_t	 kmem_align_t;
typedef union kmem_block kmem_block_t;

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

	while(cur != NULL && (uintptr_t)cur < (uintptr_t)block) {
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
	uintptr_t s = (uintptr_t)start;
	uintptr_t e = (uintptr_t)end;

	s = (s + KMEM_ALIGN - 1) & ~(KMEM_ALIGN - 1);
	e &= ~(KMEM_ALIGN - 1);

	kmem_free_list = NULL;

	size = (size_t)(e - s);

	block	      = (kmem_block_t*)s;
	block->s.size = size;
	block->s.next = NULL;
	block->s.prev = NULL;

	kmem_free_list = block;
}

void* kmalloc(size_t size) {
	kmem_block_t* block;
	kmem_block_t* split;
	size_t	      total;

	if(size == 0) return NULL;

	if(size > (size_t)-1 - KMEM_HEADER - KMEM_ALIGN) return NULL;

	size  = kmem_round(size);
	total = KMEM_HEADER + size;

	for(block = kmem_free_list; block != NULL; block = block->s.next) {
		if(block->s.size < total) continue;

		kmem_remove(block);

		if(block->s.size >= total + KMEM_HEADER + KMEM_ALIGN) {
			split = (kmem_block_t*)((char*)block + total);

			split->s.size = block->s.size - total;
			split->s.next = NULL;
			split->s.prev = NULL;

			block->s.size = total;

			kmem_insert(split);
		}

		return (void*)(block + 1);
	}

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

	if(ptr == NULL) return;

	block = ((kmem_block_t*)ptr) - 1;

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
}
