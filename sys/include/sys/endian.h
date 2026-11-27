#ifndef __SYS_ENDIAN_H__
#define __SYS_ENDIAN_H__

#include <sys/cdefs.h>
#include <sys/bswap.h>

#ifdef __LITTLE_ENDIAN__
#define ltohs(x) (x)
#define ltohl(x) (x)
#define htols(x) (x)
#define htoll(x) (x)

#define btohs(x) bswap16(x)
#define btohl(x) bswap32(x)
#define htobs(x) bswap16(x)
#define htobl(x) bswap32(x)
#else
#define ltohs(x) bswap16((x))
#define ltohl(x) bswap32((x))
#define htols(x) bswap16((x))
#define htoll(x) bswap32((x))

#define btohs(x) (x)
#define btohl(x) (x)
#define htobs(x) (x)
#define htobl(x) (x)
#endif

#endif
