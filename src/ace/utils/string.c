/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/string.h>

char *stringDecimalFromULong(ULONG ulVal, char *pDst) {
	// Modified from https://github.com/german-one/itostr/blob/master/itostr.c
	// Relies on prior verification that buffer != NULL and bufsize != 0
	// TODO: add ACE checks for that

  char* pEnd = pDst; // Pointer to the position to write
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
  while (pDst < pEnd) {
		// Reverse the digits (only the digits) in the array because they are
		// LTR written but we need them in RTL order
    char transfer = *pEnd;
    *pEnd-- = *pDst;
    *pDst++ = transfer;
  }
  return pWriteEnd;
}

void strToUpper(const char *szSrc, char *szDst) {
	while(*szSrc) {
		char c = *(szSrc++);
		if('a' <= c && c <= 'z') {
			c -= 'a' - 'A';
		}
		*(szDst++) = c;
	}
	*szDst = '\0';
}

UBYTE stringIsEmpty(const char *szStr) {
	UBYTE isEmpty = (szStr[0] == '\0');
	return isEmpty;
}
