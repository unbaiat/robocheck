
#include <stdio.h>
#include <stdlib.h>

#include "rbc_splint.h"

struct rbc_output **
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count)
{
	struct rbc_static_input *static_input = NULL;
	struct rbc_output **output = NULL;

	*err_count = 0;

	if (input != NULL && input->tool_type == STATIC_TOOL)
	{
		static_input = (struct rbc_static_input *) input->input_ptr;
		if (static_input != NULL)
		{
			printf ("HELLO FROM SPLINT\n");
		}
	}

	// TEST
	output = (struct rbc_output **)malloc(sizeof(**output));
	output[0] = (struct rbc_output *)malloc(sizeof *output);
	output[0]->err_type = ERR_SPL_NULL_DEREF;

	return output;
}

void *
parse (enum EN_err_type err_code)
{
	return NULL;
}

void *
fill_err_type  (enum EN_err_type err_code)
{
	return NULL;
}
