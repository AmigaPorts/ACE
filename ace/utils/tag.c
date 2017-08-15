#include <ace/utils/tag.h>
#include <ace/managers/log.h>

tTag tagGet(void *pTagListPtr, va_list vaSrcList, tTag ulTagToFind, ULONG ulOnNotFound) {
	if(pTagListPtr) {
		// TODO
		logWrite("ERR: Unimplemented in tagFindString()");
		return ulOnNotFound;
	}

	va_list vaWorkList;
	va_copy(vaWorkList, vaSrcList);
	tTag ulTagName;
	do {
		ulTagName = va_arg(vaWorkList, tTag);
		if(ulTagName == ulTagToFind) {
			ULONG ulOut = va_arg(vaWorkList, ULONG);
			va_end(vaWorkList);
			return ulOut;
		}
		else
			va_arg(vaWorkList, ULONG);
	} while(ulTagName != TAG_DONE);
	va_end(vaWorkList);
	return ulOnNotFound;
}
