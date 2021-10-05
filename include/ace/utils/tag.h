/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_TAG_H_
#define _ACE_UTILS_TAG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Util for handling AmigaOS tag list pattern.
 */

#include <stdarg.h>
#include <ace/types.h>

// This is implemented on AOS 2.0+ (utility/tagitem.h) and I could include it
// But I can't detect if we're building for 1.3. Even if I could do that
// I'm overriding type name for ACE convention, so I just define needed stuff.
// Ifdef is needed because of e.g. utils/bitmap.h includes graphics_protos.h
// which may (or may not on KS 1.3) in turn include tag defines.
typedef ULONG tTag;
#ifndef TAG_DONE
#define TAG_DONE   0UL
#define TAG_END    0UL
#define TAG_IGNORE 1UL
#define TAG_MORE   2UL
#define TAG_SKIP   3UL
#define TAG_USER   BV(31)
#endif // TAG_DONE

/**
 *  Finds and returns value of specified tag name from tag list.
 *  Tag list may be supplied as list or va_list.
 *  TODO: 1st arg doesn't work yet
 *  @param pTagListPtr  Pointer to tag list.
 *  @param vaSrcList    va_list containing alternating tags and values.
 *  @param ulTagToFind  Tag name, of which value should be returned.
 *  @param ulOnNotFound Value to be returned if tag was not found on list.
 *  @return ulOnNotFound if tag was not found, otherwise tag value.
 */
ULONG tagGet(
	void *pTagListPtr, va_list vaSrcList, tTag ulTagToFind, ULONG ulOnNotFound
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_TAG_H_
