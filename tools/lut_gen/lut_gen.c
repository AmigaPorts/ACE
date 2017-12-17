#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define FIXMATH_USE_FLOATS
#include "../../include/fixmath/fixmath.h"

#define PI 3.14159265358979323846264338327950288f

int main(int argc, char* argv[]) {
	
	uint32_t ulAngleCount = 64;
	
	FILE *pHeader = fopen("fix16_lut.h", "wb");
	fprintf(pHeader, "#ifndef __fix16_lut_h__\n");
	fprintf(pHeader, "#define __fix16_lut_h__\n\n");
	
	fprintf(pHeader, "#define FIX16_LUT_SIZE %d\n\n", ulAngleCount);
	fprintf(
		pHeader, "#define fix16_sin_lut(x) _fix16_sin_lut[(x) %% FIX16_LUT_SIZE]\n"
	);
	fprintf(
		pHeader, "#define fix16_cos_lut(x) _fix16_sin_lut[((x) + (FIX16_LUT_SIZE/4)) %% FIX16_LUT_SIZE]\n\n"
	);
	
	fprintf(pHeader, "static uint16_t _fix16_sin_lut[FIX16_LUT_SIZE] = {\n\t");
	for(uint32_t i = 0; i < ulAngleCount; ++i) {
		float fAngle = (2*PI*i) / ulAngleCount;
		float s = sinf(fAngle);
		fix16_t fix = fix16_from_float(s);
		fprintf(pHeader, "%d", fix);
		if(i < ulAngleCount-1) {
			fputs(",", pHeader);
			if(i && !(i % 8))
				fputs("\n\t", pHeader);
			else
				fputs(" ", pHeader);
		}
		else {
			fputs("\n", pHeader);
		}
	}
	fputs("};\n", pHeader);
	
	fputs("\n#endif // __fix16_lut_h__\n", pHeader);
	fclose(pHeader);
	
	return 1;
}