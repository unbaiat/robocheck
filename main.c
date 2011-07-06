
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/librobocheck.h"

int main(int argc, char **argv)
{
	init_robocheck();
	run_robocheck();
	close_robocheck();

	return 0;
}

