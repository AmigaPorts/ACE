#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>

tFile *fileOpen(const char *szPath, const char *szMode) {
	systemUse();
	return fopen(szPath, szMode);
}

void fileClose(tFile *pFile) {
	fclose(pFile);
	systemUnuse();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
	return fread(pDest, ulSize, 1, pFile);
}

ULONG fileWrite(tFile *pFile, void *pSrc, ULONG ulSize) {
	return fwrite(pSrc, ulSize, 1, pFile);
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	return fseek(pFile, ulPos, wMode);
}

ULONG fileGetPos(tFile *pFile) {
	return ftell(pFile);
}

UBYTE fileIsEof(tFile *pFile) {
	return feof(pFile);
}

LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	return vfprintf(pFile, szFmt, vaArgs);
}

LONG filePrintf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}

void fileFlush(tFile *pFile) {
	fflush(pFile);
}
