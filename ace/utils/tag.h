#ifndef GUARD_ACE_UTILS_TAG_H
#define GUARD_ACE_UTILS_TAG_H

/**
 * Util for handling AmigaOS tag list pattern.
 */

#include <ace/types.h>
#include <utility/tagitem.h>
#include <stdarg.h>

typedef Tag tTag;

/**
 *  Finds and returns value of specified tag name from tag list.
 *  Tag list may be supplied as list or va_list.
 *  TODO: 1st arg doesn't work yet
 *  @param pTagListPtr  Pointer to tag list.
 *  @param vaSrcList    va_list containing alternating tags and values.
 *  @param ulTagToFind  Tag name, of which value should be returned.
 *  @param ulOnNotFound Value to be returned if tag was not found on list.
 *  @return Zero if tag was not found, otherwise tag value.
 */
ULONG tagGet(
	IN void *pTagListPtr,
	IN va_list vaSrcList,
	IN tTag ulTagToFind,
	IN ULONG ulOnNotFound
);

#endif // GUARD_ACE_UTILS_TAG_H
