# Code style

## Tabs, braces and indents

``` c
void someFn(void) {
  if(stuff) {

  }
  else {

  }

  // the only case when control statement body is in same line
  while(someEvent()) continue;

  do {
    // stuff
  } while(condition);

  switch(var) {
    case
  }
}
```

- We use one true brace, with `else` being in next line.

- Write braces at all times. Although ommiting braces allows writing more compact and cleaner code, there is one major flaw - expanding statement to block after control expression makes line with condition being highlighted in diff, even if condition hasn't changed.

- @approxit likes tab sized to 4 spaces, @tehKaiN likes 2 spaces, so we're having source tab-based and its size set to different size in one's IDE.

## Code folding

Code is horizontally limited to 80 chars, so naturally some code folding must be done.
For functions in C files:

``` c
void fnShort(t1 arg1, t2 fnNameAndArgsFitInOneLine);

void fnWithManyArgs(
  t1 arg1, t2 arg2, t3 allArgsFitInOneLineButWithoutFnName
);

void fnWithTooManyArgs(
  t1 arg1, t2 arg2, t3 thereAreTooManyFnArgsToFitInOneLine,
  t4 arg4, t5 arg5, t6 soArgListIsBrokenToMultipleLines
);
```

Other arg formatting stuff doesn't mix with variable tab size properly. So I guess it must stay this way.

## Naming conventions

camelCase with prefixes. List of prefixes:

- `ub` - unsigned byte (`UBYTE`, `uint8_t`)
- `uw` - unsigned word (`UWORD`, `uint16_t`)
- `ul` - unsigned long (`ULONG`, `uint32_t`)
- `ull` - unsigned long long (`uint64_t`)
-`b`, `w`, `l`, `ll` for signed variants
- `p` - any pointer or array
- `cb` - any function pointer (callback)
- `t` - typedef
- `e` - enum instance
- `s` - struct instance
- `u` - union instance
- `f` - `float` or fixed-point var
- `d` - double

for global var scoping additionally:

- `g_` - global visible from other files
- `s_` - global visible from current file (`static`)

## Misc stuff

- all named structs and unions should be typedefed using following convention:

```c
typedef struct _tTypeName {

} tTypeName;
```

- all fns in header files **must** be documented using DoxyComments. Functions which are not exported in headers _should_ be documented in .c file

- functions should be as short as possible. If needed, split function to multiple ones. There is no hard limit for fn length, but if it doesn't fit entirely on your screen then it's a good sign you're doing something wrong.

## Include guards

Consider following example:

``` c
#ifndef _ACE_MANAGERS_BLIT_H_
#define _ACE_MANAGERS_BLIT_H_

// stuff

#endif // _ACE_MANAGERS_BLIT_H_
```

A guard is preceded by project name (ACE) and following parts strictly reflect filesystem location (inc/**managers/blit.h**).

## Includes

- if you're in .c/.cpp file, the first include is one complementing the source file (e.g. foo.h if you're in foo.c)
- then goes the standard library,
- then goes ACE - try to put managers first, then utils;
- then includes of your project files.
- No newlines between include directives.

Example for main.c:

```c
#include "main.h"
#include <stdlib.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/utils/palette.h>
#include "menu/menu.h"
#include "input.h"
```

## Doxygen

- Use javadoc (`@section` instead of `\section`) style.
- Always put `@file` doxy comment right after include guard.
- Always document each globally-visible function in .h file.
- Document `static` functions in .c files.
