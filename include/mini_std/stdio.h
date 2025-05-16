#ifndef _MINI_STD_STDIO_H_
#define _MINI_STD_STDIO_H_

#include <stddef.h>
#include <stdarg.h>
#include "printf.h"

#ifdef __cplusplus
#if !defined(restrict)
#define restrict
#endif
extern "C" {
#endif

typedef int FILE; // Whatever, it's using pointer anyway

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define EOF -1
#define FILENAME_MAX 1024

FILE *fopen(const char *restrict filename, const char *restrict mode);

size_t fread(void *restrict buffer, size_t size, size_t count, FILE *restrict stream);

size_t fwrite(const void *restrict buffer, size_t size, size_t count, FILE *restrict stream);

int fclose(FILE *stream);

int fseek(FILE *stream, long offset, int origin);

int fflush( FILE *stream );

long ftell( FILE *stream );

int feof( FILE *stream );

static inline int vsprintf(char *restrict buffer, const char *restrict format, va_list vlist) {
	return vsnprintf(buffer, 65535, format, vlist);
}

int rename(const char *szSource, const char *szDestination);

int remove(const char* szFilePath);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif // _MINI_STD_STDIO_H_
