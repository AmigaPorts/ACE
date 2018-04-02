#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>

tFile *fileOpen(const char *szPath, const char *szMode) {
	systemUse();
	FILE *pFile = fopen(szPath, szMode);
	systemUnuse();
	return pFile;
}

void fileClose(tFile *pFile) {
	systemUse();
	fclose(pFile);
	systemUnuse();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
	systemUse();
	ULONG ulResult = fread(pDest, ulSize, 1, pFile);
	systemUnuse();
	return ulResult;
}

ULONG fileWrite(tFile *pFile, void *pSrc, ULONG ulSize) {
	systemUse();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	systemUnuse();
	return ulResult;
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	systemUse();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	systemUnuse();
	return ulResult;
}

ULONG fileGetPos(tFile *pFile) {
	systemUse();
	ULONG ulResult = ftell(pFile);
	systemUnuse();
	return ulResult;
}

UBYTE fileIsEof(tFile *pFile) {
	systemUse();
	UBYTE ubResult = feof(pFile);
	systemUnuse();
	return ubResult;
}

LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	systemUse();
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	fflush(pFile);
	systemUnuse();
	return lResult;
}

LONG filePrintf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = fileVaPrintf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}

void fileFlush(tFile *pFile) {
	systemUse();
	fflush(pFile);
	systemUnuse();
}
