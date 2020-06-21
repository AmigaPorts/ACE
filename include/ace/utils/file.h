/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_FILE_H_
#define _ACE_UTILS_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <ace/types.h>

#define FILE_SEEK_CURRENT SEEK_CUR
#define FILE_SEEK_SET SEEK_SET
#define FILE_SEEK_END SEEK_END

typedef FILE tFile;

tFile *fileOpen(const char *szPath, const char *szMode);

void fileClose(tFile *pFile);

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize);

ULONG fileWrite(tFile *pFile, void *pSrc, ULONG ulSize);

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode);

ULONG fileGetPos(tFile *pFile);

UBYTE fileIsEof(tFile *pFile);

LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs);

LONG filePrintf(tFile *pFile, const char *szFmt, ...);

LONG fileVaScanf(tFile *pFile, const char *szFmt, va_list vaArgs);

LONG fileScanf(tFile *pFile,const char *szFmt, ...);

void fileFlush(tFile *pFile);

/**
 * @brief Returns file size of file, in bytes.
 *
 * @param szPath Path to file.
 * @return LONG On fail -1, otherwise file size in bytes.
 */
LONG fileGetSize(const char *szPath);

void fileWriteStr(tFile *pFile, const char *szLine);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_FILE_H_
