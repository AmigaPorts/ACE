#include <stdint.h>

__attribute__ ((used))
int __clzsi2 (unsigned x) {
	// https://en.wikipedia.org/wiki/Find_first_set#CLZ - using clz3
	if(x == 0) {
		return 32;
	}
	int n = 0;
	if((x & 0xFFFF0000) == 0){
		n += 16;
		x <<= 16;
	}
	if((x & 0xFF000000) == 0){
		n += 8;
		x <<= 8;
	}
	if((x & 0xF0000000) == 0){
		n += 4;
		x <<= 4;
	}
	if((x & 0xC0000000) == 0){
		n += 2;
		x <<= 2;
	}
	if((x & 0x80000000) == 0){
		n += 1;
	}
	return n;
}

// https://elixir.bootlin.com/linux/latest/source/include/linux/compiler_attributes.h#L173
#define mode(x) __attribute__((__mode__(x)))

// https://elixir.bootlin.com/linux/latest/source/arch/m68k/lib/muldi3.c
#define SI_TYPE_SIZE 32
#define BITS4 (SI_TYPE_SIZE / 4)
#define ll_B (1L << (SI_TYPE_SIZE / 2))
#define ll_lowpart(t) ((USItype) (t) % ll_B)
#define ll_highpart(t) ((USItype) (t) / ll_B)

#define umul_ppmm(w1, w0, u, v) do {                            \
	USItype x0, x1, x2, x3;                                       \
	USItype ul, vl, uh, vh;                                       \
	                                                              \
	ul = ll_lowpart (u);	                                        \
	uh = ll_highpart (u);                                         \
	vl = ll_lowpart (v);                                          \
	vh = ll_highpart (v);                                         \
	                                                              \
	x0 = (USItype) ul * vl;                                       \
	x1 = (USItype) ul * vh;                                       \
	x2 = (USItype) uh * vl;                                       \
	x3 = (USItype) uh * vh;                                       \
	                                                              \
	x1 += ll_highpart (x0); /* this can't give carry */           \
	x1 += x2;		            /* but this indeed can */		          \
	if (x1 < x2)		        /* did we get it? */			            \
		x3 += ll_B;		        /* yes, add it in the proper pos. */	\
	                                                              \
	(w1) = x3 + ll_highpart (x1);                                 \
	(w0) = ll_lowpart (x1) * ll_B + ll_lowpart (x0);              \
} while (0)

#define __umulsidi3(u, v) ({           \
	DIunion w;                           \
  umul_ppmm (w.s.high, w.s.low, u, v); \
  w.ll;                                \
})

typedef int SItype mode(SI);
typedef unsigned int USItype mode(SI);
typedef int DItype mode(DI);
typedef int word_type mode(__word__);

typedef union {
	struct {SItype high, low;} s;
	DItype ll;
} DIunion;

__attribute__ ((used))
DItype __muldi3 (DItype u, DItype v) {
  DIunion w, uu, vv;

  uu.ll = u,
  vv.ll = v;

  w.ll = __umulsidi3 (uu.s.low, vv.s.low);
  w.s.high += (
		(USItype) uu.s.low * (USItype) vv.s.high +
		(USItype) uu.s.high * (USItype) vv.s.low
	);

  return w.ll;
}
