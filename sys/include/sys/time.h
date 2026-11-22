#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__

#include <sys/cdefs.h>
#include <sys/types.h>

typedef long time_t;

struct timeval {
	time_t tv_sec;
	long   tv_usec;
};

struct timespec {
	time_t tv_sec;
	long   tv_nsec;
};

__BEGIN_EXTERN_C__

extern int hz;

extern volatile unsigned long ticks;

void	      microtime(struct timeval* tv);
unsigned long ticktime(unsigned int ms);

__END_EXTERN_C__

#endif
