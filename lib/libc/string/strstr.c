#include <string.h>

char *strstr(const char *haystack, const char *needle){
	const char* p1 = haystack;
	const char* p2;

	while(*haystack){
		p2 = needle;

		while(*p2 && (*p1 == *p2)){
			++p1;
			++p2;
		}

		if(*p2 == 0) return (char*)haystack;

		++haystack;
		p1 = haystack;
	}

	return NULL;
}
