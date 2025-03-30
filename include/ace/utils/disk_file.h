/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_DISK_FILE_H_
#define _ACE_UTILS_DISK_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "file.h"

typedef enum tDiskFileMode {
	DISK_FILE_MODE_READ,
	DISK_FILE_MODE_WRITE,
} tDiskFileMode;

/**
 * @brief Opens the filesystem file for read/write.
 *
 * @param szPath Path to file to be opened.
 * @param eMode File open mode - see tDiskFileMode
 * @param isUninterrupted Set to 1 if file won't remain opened while
 * you enable/disable OS or meddle with blitter ownership.
 * Speeds up the read/write operations.
 * @return File handle on success, zero on failure.
 */
tFile *diskFileOpen(const char *szPath, tDiskFileMode eMode, UBYTE isUninterrupted);

/**
 * @brief Check whether file at given path exists and is not a directory.
 *
 * @param szPath Path to file to be checked.
 * @return Success: 1, otherwise 0.
 *
 * @see dirExists()
 */
UBYTE diskFileExists(const char *szPath);

/**
 * @brief Deletes the selected file.
 *
 * @param szPath Path to file to be deleted.
 * @return 1 on success, otherwise 0, including if file does not exist.
 */
UBYTE diskFileDelete(const char *szPath);

/**
 * @brief Moves or renames selected file into another file.
 *
 * @param szSource Path to source file to be moved.
 * @param szDest Path to new file destination.
 * @return 1 on success, otherwise 0, including if source file doesn't exist or destination path is already occupied.
 */
UBYTE diskFileMove(const char *szSource, const char *szDest);


#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_DISK_FILE_H_
