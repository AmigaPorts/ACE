/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_PAK_FILE_H_
#define _ACE_UTILS_PAK_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "file.h"

#if !defined(ACE_FILE_USE_ONLY_DISK)

typedef struct tPakFileEntry {
	ULONG ulOffs;
	ULONG ulSizeUncompressed;
	ULONG ulSizeData;
	ULONG ulPathChecksum; // adler32
} tPakFileEntry;

typedef struct tPakFile {
	tFile *pFile;
	void *pPrevReadSubfile;
	UWORD uwFileCount;
	tPakFileEntry *pEntries;
} tPakFile;

tPakFile *pakFileOpen(const char *szPath, UBYTE isUninterrupted);

void pakFileClose(tPakFile *pPakFile);

tFile *pakFileGetFile(tPakFile *pPakFile, const char *szInternalPath);

#endif

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_PAK_FILE_H_
