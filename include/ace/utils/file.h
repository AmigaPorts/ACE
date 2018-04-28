/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_UTILS_FILE_H
#define GUARD_ACE_UTILS_FILE_H

#include <stdio.h>
#include <ace/types.h>

#define FILE_SEEK_CURRENT SEEK_CUR
#define FILE_SEEK_SET SEEK_SET
#define FILE_SEEK_END SEEK_END

typedef FILE tFile;

tFile *fileOpen(
	IN const char *szPath,
	IN const char *szMode
);

void fileClose(
	IN tFile *pFile
);

ULONG fileRead(
	IN tFile *pFile,
	OUT void *pDest,
	IN ULONG ulSize
);

ULONG fileWrite(
	IN tFile *pFile,
	IN void *pSrc,
	IN ULONG ulSize
);

ULONG fileSeek(
	IN tFile *pFile,
	IN ULONG ulPos,
	IN WORD wMode
);

ULONG fileGetPos(
	IN tFile *pFile
);

UBYTE fileIsEof(
	IN tFile *pFile
);

LONG fileVaPrintf(
	IN tFile *pFile,
	IN const char *szFmt,
	IN va_list vaArgs
);

LONG filePrintf(
	IN tFile *pFile,
	IN const char *szFmt,
	IN ...
);

LONG fileVaScanf(
	IN tFile *pFile,
	IN const char *szFmt,
	OUT va_list vaArgs
);

LONG fileScanf(
	IN tFile *pFile,
	IN const char *szFmt,
	OUT ...
);

void fileFlush(
	IN tFile *pFile
);

#endif // GUARD_ACE_UTILS_FILE_H
