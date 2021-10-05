#ifndef _ACE_UTILS_MINI_STD_H_
#define _ACE_UTILS_MINI_STD_H_

// All the missing stuff that ACE needs and which Bartman's GCC doesn't provide.

#include <stddef.h>
#include <stdarg.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef int FILE;

size_t strlen(const char *szStr);

int vsprintf(char *szOut, const char *szFmt, va_list vaArgs);

char *strcpy(char *szDest, const char *szSrc);

char *strcat(char *szDest, const char *szSrc);

int fseek(FILE * stream, long int offset, int origin);

#endif // _ACE_UTILS_MINI_STD_H_
