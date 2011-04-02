#include <stdio.h>
#include <stdlib.h>
#include "rbc_penalty.h"

void
splint_null_deref_handler (void * aux_err_info)
{
	printf ("Hello from null deref handler");
	printf ("\nThis is a second message after compiling only the penalty.so\n");
}
