
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/librobocheck.h"

int main(int argc, char **argv)
{
	FILE *logger = stderr;

	init_robocheck(logger);
	run_robocheck();
	close_robocheck();

	return 0;
}

