#ifndef __STRING_H__
#define __STRING_H__

#include <sys/cdefs.h>

#include <stddef.h>

__BEGIN_EXTERN_C__

void* memchr(const void* s, int c, size_t n);
int   memcmp(const void* s1, const void* s2, size_t n);
void* memcpy(void* s1, const void* s2, size_t n);
void* memmove(void* s1, const void* s2, size_t n);
void* memset(void* s, int c, size_t n);

char*  strcat(char* s1, const char* s2);
char*  strchr(const char* s1, int c);
int    strcmp(const char* s1, const char* s2);
char*  strcpy(char* s1, const char* s2);
char*  strdup(const char* s);
size_t strlen(const char* s);
char*  strstr(const char* haystack, const char* needle);
char*  strrchr(const char* s1, int c);

char* strerror(int errnum);

__END_EXTERN_C__

#endif
