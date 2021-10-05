#ifndef _MINI_STD_STDLIB_H_
#define _MINI_STD_STDLIB_H_

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void exit(int exit_code);

void qsort(
	void *ptr, size_t count, size_t size, int (*comp)(const void *, const void *)
);

#endif // _MINI_STD_STDLIB_H_
