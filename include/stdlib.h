#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <sys/cdefs.h>
#include <stddef.h>

typedef void* (*libc_malloc_t)(size_t size);
typedef void* (*libc_realloc_t)(void* ptr, size_t size);
typedef void (*libc_free_t)(void* ptr);

__BEGIN_EXTERN_C__

int  atoi(const char* nptr);
long atol(const char* nptr);

long		   strtol(const char* nptr, char** endptr, int base);
long long	   strtoll(const char* nptr, char** endptr, int base);
unsigned long	   strtoul(const char* nptr, char** endptr, int base);
unsigned long long strtoull(const char* nptr, char** endptr, int base);

extern libc_malloc_t  libc_malloc;
extern libc_realloc_t libc_realloc;
extern libc_free_t    libc_free;

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void  free(void* ptr);

__END_EXTERN_C__

#endif
