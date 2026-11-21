/*
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <dyntypes.h>
#include <string.h>

#define DYNTYPES_STATS(x)
#define DYNTYPES_ASSERT(x)

//
// dyntypes_arr implementation
//

// int *prev_allocs[65536];
// int num_prev;

void* dyntypes_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap) {
	dyntypes_array_header temp = {0}; // force debugging
	void*		      b;
	size_t		      min_len = dyntypes_arrlen(a) + addlen;
	(void)sizeof(temp);

	// compute the minimum capacity needed
	if(min_len > min_cap)
		min_cap = min_len;

	if(min_cap <= dyntypes_arrcap(a))
		return a;

	// increase needed capacity to guarantee O(1) amortized
	if(min_cap < 2 * dyntypes_arrcap(a))
		min_cap = 2 * dyntypes_arrcap(a);
	else if(min_cap < 4)
		min_cap = 4;

	// if (num_prev < 65536) if (a) prev_allocs[num_prev++] = (int *) ((char *) a+1);
	// if (num_prev == 2201)
	//   num_prev = num_prev;
	b = DYNTYPES_REALLOC(NULL, (a) ? dyntypes_header(a) : 0, elemsize * min_cap + sizeof(dyntypes_array_header));
	// if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b;
	b = (char*)b + sizeof(dyntypes_array_header);
	if(a == NULL) {
		dyntypes_header(b)->length     = 0;
		dyntypes_header(b)->hash_table = 0;
		dyntypes_header(b)->temp       = 0;
	} else {
		DYNTYPES_STATS(++dyntypes_array_grow);
	}
	dyntypes_header(b)->capacity = min_cap;

	return b;
}

void dyntypes_arrfreef(void* a) {
	DYNTYPES_FREE(NULL, dyntypes_header(a));
}

//
// dyntypes_hm hash table implementation
//

#ifdef DYNTYPES_INTERNAL_SMALL_BUCKET
#define DYNTYPES_BUCKET_LENGTH 4
#else
#define DYNTYPES_BUCKET_LENGTH 8
#endif

#define DYNTYPES_BUCKET_SHIFT (DYNTYPES_BUCKET_LENGTH == 8 ? 3 : 2)
#define DYNTYPES_BUCKET_MASK (DYNTYPES_BUCKET_LENGTH - 1)
#define DYNTYPES_CACHE_LINE_SIZE 64

#define DYNTYPES_ALIGN_FWD(n, a) (((n) + (a) - 1) & ~((a) - 1))

typedef struct
{
	size_t	  hash[DYNTYPES_BUCKET_LENGTH];
	ptrdiff_t index[DYNTYPES_BUCKET_LENGTH];
} dyntypes_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one 64-byte cache line

typedef struct
{
	char*		      temp_key; // this MUST be the first field of the hash table
	size_t		      slot_count;
	size_t		      used_count;
	size_t		      used_count_threshold;
	size_t		      used_count_shrink_threshold;
	size_t		      tombstone_count;
	size_t		      tombstone_count_threshold;
	size_t		      seed;
	size_t		      slot_count_log2;
	dyntypes_string_arena string;
	dyntypes_hash_bucket* storage; // not a separate allocation, just 64-byte aligned storage after this struct
} dyntypes_hash_index;

#define DYNTYPES_INDEX_EMPTY -1
#define DYNTYPES_INDEX_DELETED -2
#define DYNTYPES_INDEX_IN_USE(x) ((x) >= 0)

#define DYNTYPES_HASH_EMPTY 0
#define DYNTYPES_HASH_DELETED 1

static size_t dyntypes_hash_seed = 0x31415926;

void dyntypes_rand_seed(size_t seed) {
	dyntypes_hash_seed = seed;
}

#define dyntypes_load_32_or_64(var, temp, v32, v64_hi, v64_lo) \
	temp	= v64_lo ^ v32, temp <<= 16, temp <<= 16, temp >>= 16, temp >>= 16, /* discard if 32-bit */ \
	    var = v64_hi, var <<= 16, var <<= 16,				    /* discard if 32-bit */ \
	    var ^= temp ^ v32

#define DYNTYPES_SIZE_T_BITS ((sizeof(size_t)) * 8)

static size_t dyntypes_probe_position(size_t hash, size_t slot_count, size_t slot_log2) {
	size_t pos;
	DYNTYPES_NOTUSED(slot_log2);
	pos = hash & (slot_count - 1);
#ifdef DYNTYPES_INTERNAL_BUCKET_START
	pos &= ~DYNTYPES_BUCKET_MASK;
#endif
	return pos;
}

static size_t dyntypes_log2(size_t slot_count) {
	size_t n = 0;
	while(slot_count > 1) {
		slot_count >>= 1;
		++n;
	}
	return n;
}

static dyntypes_hash_index* dyntypes_make_hash_index(size_t slot_count, dyntypes_hash_index* ot) {
	dyntypes_hash_index* t;
	t		   = (dyntypes_hash_index*)DYNTYPES_REALLOC(NULL, 0, (slot_count >> DYNTYPES_BUCKET_SHIFT) * sizeof(dyntypes_hash_bucket) + sizeof(dyntypes_hash_index) + DYNTYPES_CACHE_LINE_SIZE - 1);
	t->storage	   = (dyntypes_hash_bucket*)DYNTYPES_ALIGN_FWD((size_t)(t + 1), DYNTYPES_CACHE_LINE_SIZE);
	t->slot_count	   = slot_count;
	t->slot_count_log2 = dyntypes_log2(slot_count);
	t->tombstone_count = 0;
	t->used_count	   = 0;

#if 0	// A1
  t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink
#elif 1 // A2
	// t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
	// t->tombstone_count_threshold   = slot_count* 3/16; // if tombstones are 3/16th of table, rebuild
	// t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink

	// compute without overflowing
	t->used_count_threshold	       = slot_count - (slot_count >> 2);
	t->tombstone_count_threshold   = (slot_count >> 3) + (slot_count >> 4);
	t->used_count_shrink_threshold = slot_count >> 2;

#elif 0 // B1
	t->used_count_threshold	       = slot_count * 13 / 16; // if 13/16th of table is occupied, grow
	t->tombstone_count_threshold   = slot_count * 2 / 16;  // if tombstones are 2/16th of table, rebuild
	t->used_count_shrink_threshold = slot_count * 5 / 16;  // if table is only 5/16th full, shrink
#else	// C1
	t->used_count_threshold	       = slot_count * 14 / 16; // if 14/16th of table is occupied, grow
	t->tombstone_count_threshold   = slot_count * 2 / 16;  // if tombstones are 2/16th of table, rebuild
	t->used_count_shrink_threshold = slot_count * 6 / 16;  // if table is only 6/16th full, shrink
#endif
	// Following statistics were measured on a Core i7-6700 @ 4.00Ghz, compiled with clang 7.0.1 -O2
	// Note that the larger tables have high variance as they were run fewer times
	//     A1            A2          B1           C1
	//    0.10ms :     0.10ms :     0.10ms :     0.11ms :      2,000 inserts creating 2K table
	//    0.96ms :     0.95ms :     0.97ms :     1.04ms :     20,000 inserts creating 20K table
	//   14.48ms :    14.46ms :    10.63ms :    11.00ms :    200,000 inserts creating 200K table
	//  195.74ms :   196.35ms :   203.69ms :   214.92ms :  2,000,000 inserts creating 2M table
	// 2193.88ms :  2209.22ms :  2285.54ms :  2437.17ms : 20,000,000 inserts creating 20M table
	//   65.27ms :    53.77ms :    65.33ms :    65.47ms : 500,000 inserts & deletes in 2K table
	//   72.78ms :    62.45ms :    71.95ms :    72.85ms : 500,000 inserts & deletes in 20K table
	//   89.47ms :    77.72ms :    96.49ms :    96.75ms : 500,000 inserts & deletes in 200K table
	//   97.58ms :    98.14ms :    97.18ms :    97.53ms : 500,000 inserts & deletes in 2M table
	//  118.61ms :   119.62ms :   120.16ms :   118.86ms : 500,000 inserts & deletes in 20M table
	//  192.11ms :   194.39ms :   196.38ms :   195.73ms : 500,000 inserts & deletes in 200M table

	if(slot_count <= DYNTYPES_BUCKET_LENGTH)
		t->used_count_shrink_threshold = 0;
	// to avoid infinite loop, we need to guarantee that at least one slot is empty and will terminate probes
	DYNTYPES_ASSERT(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
	DYNTYPES_STATS(++dyntypes_hash_alloc);
	if(ot) {
		t->string = ot->string;
		// reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any hashing
		t->seed = ot->seed;
	} else {
		size_t a, b, temp;
		memset(&t->string, 0, sizeof(t->string));
		t->seed = dyntypes_hash_seed;
		// LCG
		// in 32-bit, a =          2147001325   b =  715136305
		// in 64-bit, a = 2862933555777941757   b = 3037000493
		dyntypes_load_32_or_64(a, temp, 2147001325, 0x27bb2ee6, 0x87b0b0fd);
		dyntypes_load_32_or_64(b, temp, 715136305, 0, 0xb504f32d);
		dyntypes_hash_seed = dyntypes_hash_seed * a + b;
	}

	{
		size_t i, j;
		for(i = 0; i < slot_count >> DYNTYPES_BUCKET_SHIFT; ++i) {
			dyntypes_hash_bucket* b = &t->storage[i];
			for(j = 0; j < DYNTYPES_BUCKET_LENGTH; ++j)
				b->hash[j] = DYNTYPES_HASH_EMPTY;
			for(j = 0; j < DYNTYPES_BUCKET_LENGTH; ++j)
				b->index[j] = DYNTYPES_INDEX_EMPTY;
		}
	}

	// copy out the old data, if any
	if(ot) {
		size_t i, j;
		t->used_count = ot->used_count;
		for(i = 0; i < ot->slot_count >> DYNTYPES_BUCKET_SHIFT; ++i) {
			dyntypes_hash_bucket* ob = &ot->storage[i];
			for(j = 0; j < DYNTYPES_BUCKET_LENGTH; ++j) {
				if(DYNTYPES_INDEX_IN_USE(ob->index[j])) {
					size_t hash = ob->hash[j];
					size_t pos  = dyntypes_probe_position(hash, t->slot_count, t->slot_count_log2);
					size_t step = DYNTYPES_BUCKET_LENGTH;
					DYNTYPES_STATS(++dyntypes_rehash_items);
					for(;;) {
						size_t		      limit, z;
						dyntypes_hash_bucket* bucket;
						bucket = &t->storage[pos >> DYNTYPES_BUCKET_SHIFT];
						DYNTYPES_STATS(++dyntypes_rehash_probes);

						for(z = pos & DYNTYPES_BUCKET_MASK; z < DYNTYPES_BUCKET_LENGTH; ++z) {
							if(bucket->hash[z] == 0) {
								bucket->hash[z]	 = hash;
								bucket->index[z] = ob->index[j];
								goto done;
							}
						}

						limit = pos & DYNTYPES_BUCKET_MASK;
						for(z = 0; z < limit; ++z) {
							if(bucket->hash[z] == 0) {
								bucket->hash[z]	 = hash;
								bucket->index[z] = ob->index[j];
								goto done;
							}
						}

						pos += step; // quadratic probing
						step += DYNTYPES_BUCKET_LENGTH;
						pos &= (t->slot_count - 1);
					}
				}
			done:;
			}
		}
	}

	return t;
}

#define DYNTYPES_ROTATE_LEFT(val, n) (((val) << (n)) | ((val) >> (DYNTYPES_SIZE_T_BITS - (n))))
#define DYNTYPES_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (DYNTYPES_SIZE_T_BITS - (n))))

size_t dyntypes_hash_string(char* str, size_t seed) {
	size_t hash = seed;
	while(*str)
		hash = DYNTYPES_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;

	// Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
	hash ^= seed;
	hash = (~hash) + (hash << 18);
	hash ^= hash ^ DYNTYPES_ROTATE_RIGHT(hash, 31);
	hash = hash * 21;
	hash ^= hash ^ DYNTYPES_ROTATE_RIGHT(hash, 11);
	hash += (hash << 6);
	hash ^= DYNTYPES_ROTATE_RIGHT(hash, 22);
	return hash + seed;
}

#ifdef DYNTYPES_SIPHASH_2_4
#define DYNTYPES_SIPHASH_C_ROUNDS 2
#define DYNTYPES_SIPHASH_D_ROUNDS 4
typedef int DYNTYPES_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef DYNTYPES_SIPHASH_C_ROUNDS
#define DYNTYPES_SIPHASH_C_ROUNDS 1
#endif
#ifndef DYNTYPES_SIPHASH_D_ROUNDS
#define DYNTYPES_SIPHASH_D_ROUNDS 1
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant, for do..while(0) and sizeof()==
#endif

static size_t dyntypes_siphash_bytes(void* p, size_t len, size_t seed) {
	unsigned char* d = (unsigned char*)p;
	size_t	       i, j;
	size_t	       v0, v1, v2, v3, data;

	// hash that works on 32- or 64-bit registers without knowing which we have
	// (computes different results on 32-bit and 64-bit platform)
	// derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4 64-bit
	v0 = ((((size_t)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
	v1 = ((((size_t)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
	v2 = ((((size_t)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
	v3 = ((((size_t)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef DYNTYPES_TEST_SIPHASH_2_4
	// hardcoded with key material in the siphash test vectors
	v0 ^= 0x0706050403020100ull ^ seed;
	v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
	v2 ^= 0x0706050403020100ull ^ seed;
	v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define DYNTYPES_SIPROUND() \
	do { \
		v0 += v1; \
		v1 = DYNTYPES_ROTATE_LEFT(v1, 13); \
		v1 ^= v0; \
		v0 = DYNTYPES_ROTATE_LEFT(v0, DYNTYPES_SIZE_T_BITS / 2); \
		v2 += v3; \
		v3 = DYNTYPES_ROTATE_LEFT(v3, 16); \
		v3 ^= v2; \
		v2 += v1; \
		v1 = DYNTYPES_ROTATE_LEFT(v1, 17); \
		v1 ^= v2; \
		v2 = DYNTYPES_ROTATE_LEFT(v2, DYNTYPES_SIZE_T_BITS / 2); \
		v0 += v3; \
		v3 = DYNTYPES_ROTATE_LEFT(v3, 21); \
		v3 ^= v0; \
	} while(0)

	for(i = 0; i + sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
		data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
		data |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // discarded if size_t == 4

		v3 ^= data;
		for(j = 0; j < DYNTYPES_SIPHASH_C_ROUNDS; ++j)
			DYNTYPES_SIPROUND();
		v0 ^= data;
	}
	data = len << (DYNTYPES_SIZE_T_BITS - 8);
	switch(len - i) {
	case 7:
		data |= ((size_t)d[6] << 24) << 24; // fall through
	case 6:
		data |= ((size_t)d[5] << 20) << 20; // fall through
	case 5:
		data |= ((size_t)d[4] << 16) << 16; // fall through
	case 4:
		data |= (d[3] << 24); // fall through
	case 3:
		data |= (d[2] << 16); // fall through
	case 2:
		data |= (d[1] << 8); // fall through
	case 1:
		data |= d[0]; // fall through
	case 0:
		break;
	}
	v3 ^= data;
	for(j = 0; j < DYNTYPES_SIPHASH_C_ROUNDS; ++j)
		DYNTYPES_SIPROUND();
	v0 ^= data;
	v2 ^= 0xff;
	for(j = 0; j < DYNTYPES_SIPHASH_D_ROUNDS; ++j)
		DYNTYPES_SIPROUND();

#ifdef DYNTYPES_SIPHASH_2_4
	return v0 ^ v1 ^ v2 ^ v3;
#else
	return v1 ^ v2 ^ v3; // slightly stronger since v0^v3 in above cancels out final round operation? I tweeted at the authors of SipHash about this but they didn't reply
#endif
}

size_t dyntypes_hash_bytes(void* p, size_t len, size_t seed) {
#ifdef DYNTYPES_SIPHASH_2_4
	return dyntypes_siphash_bytes(p, len, seed);
#else
	unsigned char* d = (unsigned char*)p;

	if(len == 4) {
		unsigned int hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
#if 0
    // HASH32-A  Bob Jenkin's hash function w/o large constants
    hash ^= seed;
    hash -= (hash<<6);
    hash ^= (hash>>17);
    hash -= (hash<<9);
    hash ^= seed;
    hash ^= (hash<<4);
    hash -= (hash<<3);
    hash ^= (hash<<10);
    hash ^= (hash>>15);
#elif 1
		// HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates turned into shifts.
		// Note that converting these back to rotates makes it run a lot slower, presumably due to collisions, so I'm
		// not really sure what's going on.
		hash ^= seed;
		hash = (hash ^ 61) ^ (hash >> 16);
		hash = hash + (hash << 3);
		hash = hash ^ (hash >> 4);
		hash = hash * 0x27d4eb2d;
		hash ^= seed;
		hash = hash ^ (hash >> 15);
#else // HASH32-C   -  Murmur3
		hash ^= seed;
		hash *= 0xcc9e2d51;
		hash = (hash << 17) | (hash >> 15);
		hash *= 0x1b873593;
		hash ^= seed;
		hash = (hash << 19) | (hash >> 13);
		hash = hash * 5 + 0xe6546b64;
		hash ^= hash >> 16;
		hash *= 0x85ebca6b;
		hash ^= seed;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35;
		hash ^= hash >> 16;
#endif
		// Following statistics were measured on a Core i7-6700 @ 4.00Ghz, compiled with clang 7.0.1 -O2
		// Note that the larger tables have high variance as they were run fewer times
		//  HASH32-A   //  HASH32-BB  //  HASH32-C
		//    0.10ms   //    0.10ms   //    0.10ms :      2,000 inserts creating 2K table
		//    0.96ms   //    0.95ms   //    0.99ms :     20,000 inserts creating 20K table
		//   14.69ms   //   14.43ms   //   14.97ms :    200,000 inserts creating 200K table
		//  199.99ms   //  195.36ms   //  202.05ms :  2,000,000 inserts creating 2M table
		// 2234.84ms   // 2187.74ms   // 2240.38ms : 20,000,000 inserts creating 20M table
		//   55.68ms   //   53.72ms   //   57.31ms : 500,000 inserts & deletes in 2K table
		//   63.43ms   //   61.99ms   //   65.73ms : 500,000 inserts & deletes in 20K table
		//   80.04ms   //   77.96ms   //   81.83ms : 500,000 inserts & deletes in 200K table
		//  100.42ms   //   97.40ms   //  102.39ms : 500,000 inserts & deletes in 2M table
		//  119.71ms   //  120.59ms   //  121.63ms : 500,000 inserts & deletes in 20M table
		//  185.28ms   //  195.15ms   //  187.74ms : 500,000 inserts & deletes in 200M table
		//   15.58ms   //   14.79ms   //   15.52ms : 200,000 inserts creating 200K table with varying key spacing

		return (((size_t)hash << 16 << 16) | hash) ^ seed;
	} else if(len == 8 && sizeof(size_t) == 8) {
		size_t hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
		hash |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // avoid warning if size_t == 4
		hash ^= seed;
		hash = (~hash) + (hash << 21);
		hash ^= DYNTYPES_ROTATE_RIGHT(hash, 24);
		hash *= 265;
		hash ^= DYNTYPES_ROTATE_RIGHT(hash, 14);
		hash ^= seed;
		hash *= 21;
		hash ^= DYNTYPES_ROTATE_RIGHT(hash, 28);
		hash += (hash << 31);
		hash = (~hash) + (hash << 18);
		return hash;
	} else {
		return dyntypes_siphash_bytes(p, len, seed);
	}
#endif
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static int dyntypes_is_key_equal(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode, size_t i) {
	if(mode >= DYNTYPES_HM_STRING)
		return 0 == strcmp((char*)key, *(char**)((char*)a + elemsize * i + keyoffset));
	else
		return 0 == memcmp(key, (char*)a + elemsize * i + keyoffset, keysize);
}

#define DYNTYPES_HASH_TO_ARR(x, elemsize) ((char*)(x) - (elemsize))
#define DYNTYPES_ARR_TO_HASH(x, elemsize) ((char*)(x) + (elemsize))

#define dyntypes_hash_table(a) ((dyntypes_hash_index*)dyntypes_header(a)->hash_table)

void dyntypes_hmfree_func(void* a, size_t elemsize) {
	if(a == NULL) return;
	if(dyntypes_hash_table(a) != NULL) {
		if(dyntypes_hash_table(a)->string.mode == DYNTYPES_SH_STRDUP) {
			size_t i;
			// skip 0th element, which is default
			for(i = 1; i < dyntypes_header(a)->length; ++i)
				DYNTYPES_FREE(NULL, *(char**)((char*)a + elemsize * i));
		}
		dyntypes_strreset(&dyntypes_hash_table(a)->string);
	}
	DYNTYPES_FREE(NULL, dyntypes_header(a)->hash_table);
	DYNTYPES_FREE(NULL, dyntypes_header(a));
}

static ptrdiff_t dyntypes_hm_find_slot(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode) {
	void*		      raw_a = DYNTYPES_HASH_TO_ARR(a, elemsize);
	dyntypes_hash_index*  table = dyntypes_hash_table(raw_a);
	size_t		      hash  = mode >= DYNTYPES_HM_STRING ? dyntypes_hash_string((char*)key, table->seed) : dyntypes_hash_bytes(key, keysize, table->seed);
	size_t		      step  = DYNTYPES_BUCKET_LENGTH;
	size_t		      limit, i;
	size_t		      pos;
	dyntypes_hash_bucket* bucket;

	if(hash < 2) hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots

	pos = dyntypes_probe_position(hash, table->slot_count, table->slot_count_log2);

	for(;;) {
		DYNTYPES_STATS(++dyntypes_hash_probes);
		bucket = &table->storage[pos >> DYNTYPES_BUCKET_SHIFT];

		// start searching from pos to end of bucket, this should help performance on small hash tables that fit in cache
		for(i = pos & DYNTYPES_BUCKET_MASK; i < DYNTYPES_BUCKET_LENGTH; ++i) {
			if(bucket->hash[i] == hash) {
				if(dyntypes_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
					return (pos & ~DYNTYPES_BUCKET_MASK) + i;
				}
			} else if(bucket->hash[i] == DYNTYPES_HASH_EMPTY) {
				return -1;
			}
		}

		// search from beginning of bucket to pos
		limit = pos & DYNTYPES_BUCKET_MASK;
		for(i = 0; i < limit; ++i) {
			if(bucket->hash[i] == hash) {
				if(dyntypes_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
					return (pos & ~DYNTYPES_BUCKET_MASK) + i;
				}
			} else if(bucket->hash[i] == DYNTYPES_HASH_EMPTY) {
				return -1;
			}
		}

		// quadratic probing
		pos += step;
		step += DYNTYPES_BUCKET_LENGTH;
		pos &= (table->slot_count - 1);
	}
	/* NOTREACHED */
}

void* dyntypes_hmget_key_ts(void* a, size_t elemsize, void* key, size_t keysize, ptrdiff_t* temp, int mode) {
	size_t keyoffset = 0;
	if(a == NULL) {
		// make it non-empty so we can return a temp
		a = dyntypes_arrgrowf(0, elemsize, 0, 1);
		dyntypes_header(a)->length += 1;
		memset(a, 0, elemsize);
		*temp = DYNTYPES_INDEX_EMPTY;
		// adjust a to point after the default element
		return DYNTYPES_ARR_TO_HASH(a, elemsize);
	} else {
		dyntypes_hash_index* table;
		void*		     raw_a = DYNTYPES_HASH_TO_ARR(a, elemsize);
		// adjust a to point to the default element
		table = (dyntypes_hash_index*)dyntypes_header(raw_a)->hash_table;
		if(table == 0) {
			*temp = -1;
		} else {
			ptrdiff_t slot = dyntypes_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
			if(slot < 0) {
				*temp = DYNTYPES_INDEX_EMPTY;
			} else {
				dyntypes_hash_bucket* b = &table->storage[slot >> DYNTYPES_BUCKET_SHIFT];
				*temp			= b->index[slot & DYNTYPES_BUCKET_MASK];
			}
		}
		return a;
	}
}

void* dyntypes_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, int mode) {
	ptrdiff_t temp;
	void*	  p					 = dyntypes_hmget_key_ts(a, elemsize, key, keysize, &temp, mode);
	dyntypes_temp(DYNTYPES_HASH_TO_ARR(p, elemsize)) = temp;
	return p;
}

void* dyntypes_hmput_default(void* a, size_t elemsize) {
	// three cases:
	//   a is NULL <- allocate
	//   a has a hash table but no entries, because of shmode <- grow
	//   a has entries <- do nothing
	if(a == NULL || dyntypes_header(DYNTYPES_HASH_TO_ARR(a, elemsize))->length == 0) {
		a = dyntypes_arrgrowf(a ? DYNTYPES_HASH_TO_ARR(a, elemsize) : NULL, elemsize, 0, 1);
		dyntypes_header(a)->length += 1;
		memset(a, 0, elemsize);
		a = DYNTYPES_ARR_TO_HASH(a, elemsize);
	}
	return a;
}

static char* dyntypes_strdup(char* str);

void* dyntypes_hmput_key(void* a, size_t elemsize, void* key, size_t keysize, int mode) {
	size_t		     keyoffset = 0;
	void*		     raw_a;
	dyntypes_hash_index* table;

	if(a == NULL) {
		a = dyntypes_arrgrowf(0, elemsize, 0, 1);
		memset(a, 0, elemsize);
		dyntypes_header(a)->length += 1;
		// adjust a to point AFTER the default element
		a = DYNTYPES_ARR_TO_HASH(a, elemsize);
	}

	// adjust a to point to the default element
	raw_a = a;
	a     = DYNTYPES_HASH_TO_ARR(a, elemsize);

	table = (dyntypes_hash_index*)dyntypes_header(a)->hash_table;

	if(table == NULL || table->used_count >= table->used_count_threshold) {
		dyntypes_hash_index* nt;
		size_t		     slot_count;

		slot_count = (table == NULL) ? DYNTYPES_BUCKET_LENGTH : table->slot_count * 2;
		nt	   = dyntypes_make_hash_index(slot_count, table);
		if(table)
			DYNTYPES_FREE(NULL, table);
		else
			nt->string.mode = mode >= DYNTYPES_HM_STRING ? DYNTYPES_SH_DEFAULT : 0;
		dyntypes_header(a)->hash_table = table = nt;
		DYNTYPES_STATS(++dyntypes_hash_grow);
	}

	// we iterate hash table explicitly because we want to track if we saw a tombstone
	{
		size_t		      hash = mode >= DYNTYPES_HM_STRING ? dyntypes_hash_string((char*)key, table->seed) : dyntypes_hash_bytes(key, keysize, table->seed);
		size_t		      step = DYNTYPES_BUCKET_LENGTH;
		size_t		      pos;
		ptrdiff_t	      tombstone = -1;
		dyntypes_hash_bucket* bucket;

		// stored hash values are forbidden from being 0, so we can detect empty slots to early out quickly
		if(hash < 2) hash += 2;

		pos = dyntypes_probe_position(hash, table->slot_count, table->slot_count_log2);

		for(;;) {
			size_t limit, i;
			DYNTYPES_STATS(++dyntypes_hash_probes);
			bucket = &table->storage[pos >> DYNTYPES_BUCKET_SHIFT];

			// start searching from pos to end of bucket
			for(i = pos & DYNTYPES_BUCKET_MASK; i < DYNTYPES_BUCKET_LENGTH; ++i) {
				if(bucket->hash[i] == hash) {
					if(dyntypes_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
						dyntypes_temp(a) = bucket->index[i];
						if(mode >= DYNTYPES_HM_STRING)
							dyntypes_temp_key(a) = *(char**)((char*)raw_a + elemsize * bucket->index[i] + keyoffset);
						return DYNTYPES_ARR_TO_HASH(a, elemsize);
					}
				} else if(bucket->hash[i] == 0) {
					pos = (pos & ~DYNTYPES_BUCKET_MASK) + i;
					goto found_empty_slot;
				} else if(tombstone < 0) {
					if(bucket->index[i] == DYNTYPES_INDEX_DELETED)
						tombstone = (ptrdiff_t)((pos & ~DYNTYPES_BUCKET_MASK) + i);
				}
			}

			// search from beginning of bucket to pos
			limit = pos & DYNTYPES_BUCKET_MASK;
			for(i = 0; i < limit; ++i) {
				if(bucket->hash[i] == hash) {
					if(dyntypes_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
						dyntypes_temp(a) = bucket->index[i];
						return DYNTYPES_ARR_TO_HASH(a, elemsize);
					}
				} else if(bucket->hash[i] == 0) {
					pos = (pos & ~DYNTYPES_BUCKET_MASK) + i;
					goto found_empty_slot;
				} else if(tombstone < 0) {
					if(bucket->index[i] == DYNTYPES_INDEX_DELETED)
						tombstone = (ptrdiff_t)((pos & ~DYNTYPES_BUCKET_MASK) + i);
				}
			}

			// quadratic probing
			pos += step;
			step += DYNTYPES_BUCKET_LENGTH;
			pos &= (table->slot_count - 1);
		}
	found_empty_slot:
		if(tombstone >= 0) {
			pos = tombstone;
			--table->tombstone_count;
		}
		++table->used_count;

		{
			ptrdiff_t i = (ptrdiff_t)dyntypes_arrlen(a);
			// we want to do dyntypes_arraddn(1), but we can't use the macros since we don't have something of the right type
			if((size_t)i + 1 > dyntypes_arrcap(a))
				*(void**)&a = dyntypes_arrgrowf(a, elemsize, 1, 0);
			raw_a = DYNTYPES_ARR_TO_HASH(a, elemsize);

			DYNTYPES_ASSERT((size_t)i + 1 <= dyntypes_arrcap(a));
			dyntypes_header(a)->length		  = i + 1;
			bucket					  = &table->storage[pos >> DYNTYPES_BUCKET_SHIFT];
			bucket->hash[pos & DYNTYPES_BUCKET_MASK]  = hash;
			bucket->index[pos & DYNTYPES_BUCKET_MASK] = i - 1;
			dyntypes_temp(a)			  = i - 1;

			switch(table->string.mode) {
			case DYNTYPES_SH_STRDUP:
				dyntypes_temp_key(a) = *(char**)((char*)a + elemsize * i) = dyntypes_strdup((char*)key);
				break;
			case DYNTYPES_SH_ARENA:
				dyntypes_temp_key(a) = *(char**)((char*)a + elemsize * i) = dyntypes_stralloc(&table->string, (char*)key);
				break;
			case DYNTYPES_SH_DEFAULT:
				dyntypes_temp_key(a) = *(char**)((char*)a + elemsize * i) = (char*)key;
				break;
			default:
				memcpy((char*)a + elemsize * i, key, keysize);
				break;
			}
		}
		return DYNTYPES_ARR_TO_HASH(a, elemsize);
	}
}

void* dyntypes_shmode_func(size_t elemsize, int mode) {
	void*		     a = dyntypes_arrgrowf(0, elemsize, 0, 1);
	dyntypes_hash_index* h;
	memset(a, 0, elemsize);
	dyntypes_header(a)->length     = 1;
	dyntypes_header(a)->hash_table = h = (dyntypes_hash_index*)dyntypes_make_hash_index(DYNTYPES_BUCKET_LENGTH, NULL);
	h->string.mode			   = (unsigned char)mode;
	return DYNTYPES_ARR_TO_HASH(a, elemsize);
}

void* dyntypes_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode) {
	if(a == NULL) {
		return 0;
	} else {
		dyntypes_hash_index* table;
		void*		     raw_a = DYNTYPES_HASH_TO_ARR(a, elemsize);
		table			   = (dyntypes_hash_index*)dyntypes_header(raw_a)->hash_table;
		dyntypes_temp(raw_a)	   = 0;
		if(table == 0) {
			return a;
		} else {
			ptrdiff_t slot;
			slot = dyntypes_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
			if(slot < 0)
				return a;
			else {
				dyntypes_hash_bucket* b		  = &table->storage[slot >> DYNTYPES_BUCKET_SHIFT];
				int		      i		  = slot & DYNTYPES_BUCKET_MASK;
				ptrdiff_t	      old_index	  = b->index[i];
				ptrdiff_t	      final_index = (ptrdiff_t)dyntypes_arrlen(raw_a) - 1 - 1; // minus one for the raw_a vs a, and minus one for 'last'
				DYNTYPES_ASSERT(slot < (ptrdiff_t)table->slot_count);
				--table->used_count;
				++table->tombstone_count;
				dyntypes_temp(raw_a) = 1;
				DYNTYPES_ASSERT(table->used_count >= 0);
				// DYNTYPES_ASSERT(table->tombstone_count < table->slot_count/4);
				b->hash[i]  = DYNTYPES_HASH_DELETED;
				b->index[i] = DYNTYPES_INDEX_DELETED;

				if(mode == DYNTYPES_HM_STRING && table->string.mode == DYNTYPES_SH_STRDUP)
					DYNTYPES_FREE(NULL, *(char**)((char*)a + elemsize * old_index));

				// if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so skip
				if(old_index != final_index) {
					// swap delete
					memmove((char*)a + elemsize * old_index, (char*)a + elemsize * final_index, elemsize);

					// now find the slot for the last element
					if(mode == DYNTYPES_HM_STRING)
						slot = dyntypes_hm_find_slot(a, elemsize, *(char**)((char*)a + elemsize * old_index + keyoffset), keysize, keyoffset, mode);
					else
						slot = dyntypes_hm_find_slot(a, elemsize, (char*)a + elemsize * old_index + keyoffset, keysize, keyoffset, mode);
					DYNTYPES_ASSERT(slot >= 0);
					b = &table->storage[slot >> DYNTYPES_BUCKET_SHIFT];
					i = slot & DYNTYPES_BUCKET_MASK;
					DYNTYPES_ASSERT(b->index[i] == final_index);
					b->index[i] = old_index;
				}
				dyntypes_header(raw_a)->length -= 1;

				if(table->used_count < table->used_count_shrink_threshold && table->slot_count > DYNTYPES_BUCKET_LENGTH) {
					dyntypes_header(raw_a)->hash_table = dyntypes_make_hash_index(table->slot_count >> 1, table);
					DYNTYPES_FREE(NULL, table);
					DYNTYPES_STATS(++dyntypes_hash_shrink);
				} else if(table->tombstone_count > table->tombstone_count_threshold) {
					dyntypes_header(raw_a)->hash_table = dyntypes_make_hash_index(table->slot_count, table);
					DYNTYPES_FREE(NULL, table);
					DYNTYPES_STATS(++dyntypes_hash_rebuild);
				}

				return a;
			}
		}
	}
	/* NOTREACHED */
}

static char* dyntypes_strdup(char* str) {
	// to keep replaceable allocator simple, we don't want to use strdup.
	// rolling our own also avoids problem of strdup vs _strdup
	size_t len = strlen(str) + 1;
	char*  p   = (char*)DYNTYPES_REALLOC(NULL, 0, len);
	memmove(p, str, len);
	return p;
}

#ifndef DYNTYPES_STRING_ARENA_BLOCKSIZE_MIN
#define DYNTYPES_STRING_ARENA_BLOCKSIZE_MIN 512u
#endif
#ifndef DYNTYPES_STRING_ARENA_BLOCKSIZE_MAX
#define DYNTYPES_STRING_ARENA_BLOCKSIZE_MAX (1u << 20)
#endif

char* dyntypes_stralloc(dyntypes_string_arena* a, char* str) {
	char*  p;
	size_t len = strlen(str) + 1;
	if(len > a->remaining) {
		// compute the next blocksize
		size_t blocksize = a->block;

		// size is 512, 512, 1024, 1024, 2048, 2048, 4096, 4096, etc., so that
		// there are log(SIZE) allocations to free when we destroy the table
		blocksize = (size_t)(DYNTYPES_STRING_ARENA_BLOCKSIZE_MIN) << (blocksize >> 1);

		// if size is under 1M, advance to next blocktype
		if(blocksize < (size_t)(DYNTYPES_STRING_ARENA_BLOCKSIZE_MAX))
			++a->block;

		if(len > blocksize) {
			// if string is larger than blocksize, then just allocate the full size.
			// note that we still advance string_block so block size will continue
			// increasing, so e.g. if somebody only calls this with 1000-long strings,
			// eventually the arena will start doubling and handling those as well
			dyntypes_string_block* sb = (dyntypes_string_block*)DYNTYPES_REALLOC(NULL, 0, sizeof(*sb) - 8 + len);
			memmove(sb->storage, str, len);
			if(a->storage) {
				// insert it after the first element, so that we don't waste the space there
				sb->next	 = a->storage->next;
				a->storage->next = sb;
			} else {
				sb->next     = 0;
				a->storage   = sb;
				a->remaining = 0; // this is redundant, but good for clarity
			}
			return sb->storage;
		} else {
			dyntypes_string_block* sb = (dyntypes_string_block*)DYNTYPES_REALLOC(NULL, 0, sizeof(*sb) - 8 + blocksize);
			sb->next		  = a->storage;
			a->storage		  = sb;
			a->remaining		  = blocksize;
		}
	}

	DYNTYPES_ASSERT(len <= a->remaining);
	p = a->storage->storage + a->remaining - len;
	a->remaining -= len;
	memmove(p, str, len);
	return p;
}

void dyntypes_strreset(dyntypes_string_arena* a) {
	dyntypes_string_block *x, *y;
	x = a->storage;
	while(x) {
		y = x->next;
		DYNTYPES_FREE(NULL, x);
		x = y;
	}
	memset(a, 0, sizeof(*a));
}
