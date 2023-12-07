/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/string.h>
#include <ace/managers/log.h>
#include <ace/utils/assume.h>

char *stringDecimalFromULong(ULONG ulVal, char *szDest) {
	// Modified from https://github.com/german-one/itostr/blob/master/itostr.c
	assumeNotNull(szDest);

	char* pEnd = szDest; // Pointer to the position to write
	do {
		// Get the value of the rightmost digit
		// (avoid modulo, it would perform another slow division)
		ULONG ulQuot = ulVal / 10;
		ULONG digitval = ulVal - ulQuot * 10;
		ulVal = ulQuot;
		*pEnd++ = '0' + digitval; // Convert it to the representing character
	} while (ulVal); // Iterate as long as digits are leftover
	char *pWriteEnd = pEnd;

	// Assign the terminating null and decrease the pointer in order to point
	// To the last digit written
	*pEnd-- = '\0';
	while (szDest < pEnd) {
		// Reverse the digits (only the digits) in the array because they are
		// LTR written but we need them in RTL order
		char transfer = *pEnd;
		*pEnd-- = *szDest;
		*szDest++ = transfer;
	}
	return pWriteEnd;
}

char charToUpper(char c) {
	if('a' <= c && c <= 'z') {
		c -= 'a' - 'A';
	}
	return c;
}

void strToUpper(const char *szSrc, char *szDest) {
	assumeNotNull(szSrc);
	assumeNotNull(szDest);

	while(*szSrc) {
		char c = *(szSrc++);
		c = charToUpper(c);
		*(szDest++) = c;
	}
	*szDest = '\0';
}

UBYTE stringIsEmpty(const char *szStr) {
	assumeNotNull(szStr);

	UBYTE isEmpty = (szStr[0] == '\0');
	return isEmpty;
}

char *stringCopy(const char *szSrc, char *szDest) {
	assumeNotNull(szSrc);
	assumeNotNull(szDest);

	while(*szSrc != '\0') {
		*(szDest++) = *(szSrc++);
	};
	*szDest = '\0';
	return szDest;
}

char *stringCopyLimited(const char *szSrc, char *szDest, UWORD uwMaxLength) {
	assumeNotNull(szSrc);
	assumeNotNull(szDest);
	assumeMsg(uwMaxLength != 0, "uwMaxLength is zero");

	while(*szSrc != '\0' && --uwMaxLength > 0) {
		*(szDest++) = *(szSrc++);
		--uwMaxLength;
	};
	*szDest = '\0';
	return szDest;
}
