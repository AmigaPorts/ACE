#include <stdint.h>

int __clzsi2 (unsigned int a) {
	// https://en.wikipedia.org/wiki/Find_first_set#CLZ - using clz4
	static const uint8_t pTable[] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};
	for(uint8_t y = 1; y <= 16; y <<= 1) {
		a = a | (a >> y);
	}
	return pTable[(a * 0x07C4ACDD) >> 27];
}

#define HIWORD(x) (uint16_t)(x >> 16)
#define LOWORD(x) (uint16_t)(x & 0xFFFF)

long __muldi3 (long lAB, long lXY) {
	// Consider A,B and X,Y as 32-bit values split into hi- and lowords
	//      A  B
	//      X  Y
	// ---------
	//     Ay By <- need to only sum this
	//  Ax Bx
	//     ^^------ with this

	// Cast both args to unsigned
	uint32_t ulAB = (uint32_t)lAB;
	uint32_t ulXY = (uint32_t)lXY;
	uint32_t ulResult = (
		(LOWORD(ulAB) * LOWORD(ulXY)) +
		((HIWORD(ulAB) * LOWORD(ulXY)) << 16) +
		((HIWORD(ulXY) * LOWORD(ulAB)) << 16)
	);
	int32_t lResult = (int32_t)ulResult;
  return lResult;
}
