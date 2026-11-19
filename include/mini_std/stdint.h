#if defined(__clang__)
// useful when using clangd for completion, because that defines some random __GNUC* version defs
#include_next <stdint.h>
#else
#define MINISTD_STDINT_PATH <../../__GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__/include/stdint-gcc.h>
#include MINISTD_STDINT_PATH
#undef MINISTD_STDINT_PATH
#endif
