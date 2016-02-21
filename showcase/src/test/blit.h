#ifndef GUARD_SHOWCASE_TEST_BLIT_H
#define GUARD_SHOWCASE_TEST_BLIT_H

#include <ace/types.h>
#include <ace/config.h>

#define TYPE_RECT 0
#define TYPE_AUTO 128
#define TYPE_RAPID 64
#define TYPE_SAVEBG 32

void gsTestBlitCreate(void);
void gsTestBlitLoop(void);
void gsTestBlitDestroy(void);

#endif