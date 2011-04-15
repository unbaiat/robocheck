
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

	// TODO: hmm i think this crashes because we need to use SHM (mmap or smthg)
	printf ("%s\n", GET_RBC_ERR_ENTRY(ERR_SPL_NULL_DEREF, 0).xml_name);

	return output;
}

void
apply_penalty (struct rbc_out_info *out)
{
	if (out != NULL)
	{
	}
}

