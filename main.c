#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/librobocheck.h"

int main(int argc, char **argv)
{
	FILE *logger = stderr;
	FILE *output = stdout;

	init_robocheck(logger, output);
	run_robocheck();
	close_robocheck();

	return 0;
}

