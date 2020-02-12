#ifndef GUARD_OF_JSON_H
#define GUARD_OF_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define JSMN_STRICT       /* Strict JSON parsing */
// JSMN_PARENT_LINKS breaks things up!
// #define JSMN_PARENT_LINKS /* Speeds things up */
#include "jsmn.h"

typedef struct _tJson {
	char *szData;
	jsmntok_t *pTokens;
	uint16_t uwTokenCount;
} tJson;

tJson *jsonCreate(const char *szFilePath);

void jsonDestroy(tJson *pJson);

uint16_t jsonGetElementInArray(const tJson *pJson,uint16_t uwParentIdx,uint16_t uwIdx);

uint16_t jsonGetElementInStruct(
	const tJson *pJson,uint16_t uwParentIdx,const char *szElement
);

uint16_t jsonGetDom(const tJson *pJson,const char *szPattern);

uint32_t jsonTokToUlong(const tJson *pJson,uint16_t uwTok);

uint16_t jsonStrLen(const tJson *pJson, uint16_t uwTok);

uint16_t jsonTokStrCpy(
	const tJson *pJson, uint16_t uwTok, char *pDst, uint16_t uwMaxBytes
);

#ifdef __cplusplus
}
#endif

#endif // GUARD_OF_JSON_H
