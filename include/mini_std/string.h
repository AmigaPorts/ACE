#ifndef _MINI_STD_STRING_H_
#define _MINI_STD_STRING_H_

#include <stddef.h>
#include <../sys-include/string.h>

#ifdef __cplusplus
#if !defined(restrict)
#define restrict
#endif
extern "C" {
#endif

size_t strlen(const char *str);

char *strncpy(char *restrict dest, const char *restrict src, size_t count);

char *strcpy(char *restrict dest, const char *restrict src);

char *strcat(char *restrict dest, const char *restrict src);

char *strchr(const char *szHaystack, int cNeedle);

char *strrchr(const char *szHaystack, int cNeedle);

int memcmp(const void *lhs, const void *rhs, size_t count);

unsigned long strtoul(const char *restrict str, char **restrict str_end, int base);

int strcmp(const char *szA, const char *szB);

char *strncpy(char *restrict szDest, const char *restrict szSrc, size_t Count);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif // _MINI_STD_STRING_H_
