#ifndef GUARD_ACE_UTIL_ENDIAN_H
#define GUARD_ACE_UTIL_ENDIAN_H

#include <ace/types.h>

// TODO: Distinguish platform endian and avoid conversion if endian is matching

inline UWORD endianIntel16(UWORD uwIn) {
	return (uwIn << 8) | (uwIn >> 8);
}

inline ULONG endianIntel32(ULONG ulIn) {
	return (ulIn << 24) | ((ulIn&0xFF00) << 8) | ((ulIn & 0xFF0000) >> 8) | (ulIn >> 24);
}

#endif