#include <proto/dos.h>
#include <sort_r.h>

static int (*s_qsortComp)(const void *, const void *);

void exit(int exit_code) {
	// FIXME: implement better, Exit() shouldn't be used.
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node029F.html
	// This is what Bartman's project template uses.

	Exit(exit_code);
	while(1) continue;
}

int qsortEmu(const void *_a, const void *_b, __attribute__((unused)) void *_arg) {
	return s_qsortComp(_a, _b);
}

void qsort(
	void *ptr, size_t count, size_t size, int (*comp)(const void *, const void *)
) {
	// sort_r requires extra arg so call fixed callback which calls regular qsort cb

	s_qsortComp = comp;
	sort_r(ptr, count, size, qsortEmu, 0);
}
