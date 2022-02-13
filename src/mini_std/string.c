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
		++pLeft;
		++pRight;
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

char *strrchr(const char *szHaystack, int cNeedle) {
	// https://en.cppreference.com/w/c/string/byte/strrchr
	char *pLast = 0;
	while(*szHaystack != '\0') {
		if(*szHaystack == cNeedle) {
			pLast = (char*)szHaystack;
		}
		++szHaystack;
	}
	return pLast;
}

char *strcpy(char *restrict szDest, const char *restrict szSrc) {
	// Also copy null terminator
	memcpy(szDest, szSrc, strlen(szSrc) + 1);
	return szDest;
}

char *strcat(char *restrict szDest, const char *restrict szSrc) {
	strcpy(&szDest[strlen(szDest)], szSrc);
	return szDest;
}

int strcmp(const char *szA, const char *szB) {
	// https://en.cppreference.com/w/c/string/byte/strcmp

	// It's ok to check only bounds of szA since if szB is shorter then its null
	// terminator won't be the same as szA's char on same pos.
	while(*szA) {
		// If chars are not the same, return the difference.
		if(*szA != *szB) {
			return *szA - *szB;
		}
		++szA;
		++szB;
	}

	// szB is longer or equal to szA - return the difference.
	return *szA - *szB;
}

char *strncpy(char *restrict szDest, const char *restrict szSrc, size_t Count) {
	// https://en.cppreference.com/w/c/string/byte/strncpy

	// Calculate actual copy length.
	size_t Len = strlen(szSrc);
	if(Len > Count) {
		Len = Count;
	}

	// If the first count characters were non-null, dest will not contain a null
	// terminated string!
	memcpy(szDest, szSrc, Len);

	// Zeroes out the rest of the destination buffer, which can be
	// a performance concern.
	memset(&szDest[Len], 0, Count - Len);
	return szDest;
}
