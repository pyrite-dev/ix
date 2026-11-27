#ifndef __SYS_CDEFS_H__
#define __SYS_CDEFS_H__

#if defined(__cplusplus)
#define __BEGIN_EXTERN_C__ extern "C" {
#define __END_EXTERN_C__ }
#else
#define __BEGIN_EXTERN_C__
#define __END_EXTERN_C__
#endif

#define __packed __attribute__((__packed__))

#define __BARRIER__() __asm__ __volatile__("" ::: "memory");

#if defined(__i960__)
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#else
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#endif

#endif
