#ifndef GUARD_MANAGER_INPUT_H
#define GUARD_MANAGER_INPUT_H

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/intuition_protos.h> // IDCMP_RAWKEY etc

#include "config.h"

#include <ace/managers/window.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/joy.h>

/* Types */

/* Globals */

/* Functions */
void inputOpen(void);

void inputProcess(void);

void inputClose(void);

#endif