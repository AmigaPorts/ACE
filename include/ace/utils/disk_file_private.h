/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_DISK_FILE_PRIVATE_H_
#define _ACE_UTILS_DISK_FILE_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

#if defined(ACE_FILE_USE_ONLY_DISK)
#define DISKFILE_PRIVATE
#else
#define DISKFILE_PRIVATE static
#endif

DISKFILE_PRIVATE void diskFileClose(void *pData);
DISKFILE_PRIVATE ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize);
DISKFILE_PRIVATE ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize);
DISKFILE_PRIVATE ULONG diskFileSeek(void *pData, LONG lPos, WORD wMode);
DISKFILE_PRIVATE ULONG diskFileGetPos(void *pData);
DISKFILE_PRIVATE ULONG diskFileGetSize(void *pData);
DISKFILE_PRIVATE UBYTE diskFileIsEof(void *pData);
DISKFILE_PRIVATE void diskFileFlush(void *pData);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_DISK_FILE_PRIVATE_H_
