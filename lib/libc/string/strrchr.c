#include <string.h>

char* strrchr(const char* s1, int c) {
	size_t n = strlen(s1);

	s1 += n;
	while(n--) {
		if(*s1 == c) return (char*)s1;
		s1--;
	}

	return NULL;
}
