#include <stddef.h>
#include <../sys-include/string.h>

size_t strlen(const char *str);

char *strncpy(char *restrict dest, const char *restrict src, size_t count);

char *strcpy(char *restrict dest, const char *restrict src);

char *strcat(char *restrict dest, const char *restrict src);

char *strchr(const char *str, int ch);

int memcmp(const void *lhs, const void *rhs, size_t count);

unsigned long strtoul(const char *restrict str, char **restrict str_end, int base);
