#ifndef __SYS_BSWAP_H__
#define __SYS_BSWAP_H__

#define bswap16(x) ((unsigned short)((((x) & 0xff00) >> 8) | (((x) & 0xff) << 8)))
#define bswap32(x) ((unsigned int)(((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))))

#endif
