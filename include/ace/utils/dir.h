/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/types.h>

typedef struct _tDir {
	BPTR pLock;
	struct FileInfoBlock sFileBlock;
} tDir;

/**
 * @brief Opens directory handle for reading consecutive directory names.
 * On Amiga, this function may use OS.
 *
 * @param szPath Path to directory to be opened.
 * @return tDir* On success, directory handle, otherwise 0.
 *
 * @see dirClose()
 * @see dirRead()
 */
tDir *dirOpen(
	IN const char *szPath
);

/**
 * @brief Reads next file name in given directory to buffer of specified length.
 * After reading file name, directory handle is set to read next file.
 * On Amiga, this function may use OS.
 *
 * @param pDir Directory handle.
 * @param szFileName Destination buffer for null-terminated file name.
 * @param uwFileNameMax Length of szFileName buffer. On Amiga, OS limit for file
 *                      names is 108 chars, including null termination.
 * @return UBYTE 1 on success, otherwise false.
 *
 * @see dirOpen()
 * @see dirClose()
 */
UBYTE dirRead(
	IN tDir *pDir,
	OUT char *szFileName,
	IN UWORD uwFileNameMax
);

/**
 * @brief Closes directory handle.
 * On Amiga, this function may use OS.
 *
 * @param pDir Directory handle.
 *
 * @see dirOpen()
 */
void dirClose(
	IN tDir *pDir
);
