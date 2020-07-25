#include <stdio.h>
#include <proto/dos.h>

void _putchar(char character) {
	Write(Output(), &character, 1);
}
