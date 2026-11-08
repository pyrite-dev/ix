#ifndef __SYS_CONS_H__
#define __SYS_CONS_H__

#include <sys/cdefs.h>

struct consdev {
	void(*putc)(int c);
	int(*getc)(void);
};

__BEGIN_EXTERN_C__

void cninit(struct consdev* cons);
void cnputc(int c);
int cngetc(void);

__END_EXTERN_C__

#endif
