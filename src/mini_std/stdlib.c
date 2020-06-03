#include <proto/dos.h>

void exit(int exit_code) {
	// FIXME: implement better, Exit() shouldn't be used.
	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node029F.html
	// This is what Bartman's project template uses.

	Exit(exit_code);
	while(1) {}
}
