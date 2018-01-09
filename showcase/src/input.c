#include "input.h"
#include <ace/managers/key.h>
#include <ace/managers/joy.h>

/* Globals */

/* Functions */
void inputOpen() {
	joyOpen();
	keyCreate();
}

void inputProcess() {
	joyProcess();
	keyProcess();
}

void inputClose() {
	keyDestroy();
	joyClose();
}
