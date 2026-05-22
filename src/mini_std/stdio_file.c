#include <stdio.h>
#include <ace/types.h>
#include <proto/dos.h>

// Some things are implemented from scratch, some are based on:
// https://github.com/deplinenoise/amiga-sdk/blob/master/netinclude/stdio.h

FILE *fopen(const char *restrict szFileName, const char *restrict szMode) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0196.html
	if(!szMode) {
		return 0;
	}

	LONG lAccessMode = 0;
	while(*szMode != '\0') {
		switch(*szMode) {
			case 'r':
				lAccessMode |= MODE_OLDFILE;
				break;
			case 'w':
				lAccessMode |= MODE_NEWFILE;
				break;
			case 'b':
				// Binary - ignore, no difference here
				break;
			default:
				// Unsupported
				return 0;
		}
		++szMode;
	}

	BPTR bpFile = Open((CONST_STRPTR)szFileName, lAccessMode);
	return (FILE *)bpFile;
}

size_t fread(void *restrict pBuffer, size_t Size, size_t Count, FILE *restrict pStream) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01A0.html
	if(Size == 0 || Count == 0) return 0;
	if(Count > SIZE_MAX / Size) return 0;
	// Read() returns -1 on error (IoErr() has details)
	LONG lBytesRead = Read((BPTR)pStream, pBuffer, Size * Count);
	if(lBytesRead <= 0) return 0;
	return (size_t)lBytesRead / Size;
}

size_t fwrite(const void *restrict pBuffer, size_t Size, size_t Count, FILE *restrict pStream) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01D1.html
	if(Size == 0 || Count == 0) return 0;
	if(Count > SIZE_MAX / Size) return 0;
	LONG lBytesWritten = Write((BPTR)pStream, (void *)pBuffer, Size * Count);
	if(lBytesWritten < 0) return 0;
	return (size_t)lBytesWritten / Size;
}

int fclose(FILE *pStream) {
	// http://www.cplusplus.com/reference/cstdio/fclose/
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0149.html
	// NOTE: Close() doesn't return anything in ks1.3! So we'll just return success.
	Close((BPTR)pStream);
	return 0;
}

int fseek(FILE *pStream, long Offset, int Origin) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01AD.html
	LONG lOriginDos = OFFSET_CURRENT;
	if(Origin == SEEK_SET) {
		lOriginDos = OFFSET_BEGINNING;
	}
	else if(Origin == SEEK_END) {
		lOriginDos = OFFSET_END;
	}
	Seek((BPTR)pStream, Offset, lOriginDos);
	// FIXME: proper check for result
	return 0;
}

int fflush(UNUSED_ARG FILE *pStream) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node016A.html
	// Write is unbuffered so no need for flushing
	return 0;
}

long ftell(FILE *pStream) {
	LONG lPos = Seek((BPTR)pStream, 0L, OFFSET_CURRENT);
	return lPos;
}

int feof(FILE *pStream) {
	/* Seek returns previous position; seek-to-end moves to EOF and reports prior offset. */
	LONG lSavedPos = Seek((BPTR)pStream, 0L, OFFSET_END);
	LONG lEnd = Seek((BPTR)pStream, 0L, OFFSET_CURRENT);
	Seek((BPTR)pStream, lSavedPos, OFFSET_BEGINNING);
	return lSavedPos == lEnd;
}

int rename(const char *szSource, const char *szDestination) {
	return Rename((CONST_STRPTR)szSource, (CONST_STRPTR)szDestination) != 0;
}

int remove(const char *szFilePath) {
	return DeleteFile((CONST_STRPTR)szFilePath) != 0;
}
