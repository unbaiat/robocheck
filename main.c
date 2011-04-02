#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "librobocheck.h"

int main()
{
	int err_count = 0;
	rbc_errset_t flags;
	struct rbc_input *input = NULL;
	struct rbc_output **output = NULL;

	RESET(flags);

	input = (struct rbc_input *)malloc(sizeof *input);
	if (input != NULL) {
		input->input_ptr = (struct rbc_static_input *)malloc(sizeof (struct rbc_static_input));
		input->tool_type = STATIC_TOOL;
	}

	output = load_module(input, flags, &err_count, "libsplint.so", "run_tool");
	if (output != NULL && output[0] != NULL) {
		test_func(output[0]->err_type);
	}

	return 0;
} 

