/* sampleCodeModule.c */

#include <videoLib.h>
#include <soundLib.h>
#include <usrio.h>
#include <timeLib.h>
#include <shell.h>
#include <pongisLib.h>

char * v = (char*)0xB8000 + 79 * 2;

static int var1 = 0;
static int var2 = 0;


int main() {
	//All the following code may be removed 
	*v = 'X';
	*(v+1) = 0x74;

	startShell();

	//drawRectangle(1000, 1000, 0x00FF00, 0, 0);
	//startPongisGolf();

	//Test if BSS is properly set up
	if (var1 == 0 && var2 == 0)
		return 0xDEADC0DE;

	return 0xDEADBEEF;
}