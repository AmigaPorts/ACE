#include <string.h>

int memcmp(const void *pLhs, const void *pRhs, size_t count) {
	// https://en.cppreference.com/w/c/string/byte/memcmp
	unsigned char *pLeft = (unsigned char *)pLhs;
	unsigned char *pRight = (unsigned char *)pRhs;
	while(count--) {
		int delta = *pLeft - *pRight;
		if(delta != 0) {
			return delta;
		}
		++pLhs;
		++pRhs;
	}
	return 0;
}

unsigned long strtoul(const char *restrict str, char **restrict str_end, int base);

char *strchr(const char *szHaystack, int cNeedle) {
	// https://en.cppreference.com/w/c/string/byte/strchr
	while(*szHaystack != '\0') {
		if(*szHaystack == cNeedle) {
			return (char*)szHaystack;
		}
		++szHaystack;
	}
	return 0;
}

char *strcpy(char *restrict szDest, const char *restrict szSrc) {
	memcpy(szDest, szSrc, strlen(szSrc));
	return szDest;
}

char *strcat(char *restrict szDest, const char *restrict szSrc) {
	strcpy(&szDest[strlen(szDest)], szSrc);
	return szDest;
}
