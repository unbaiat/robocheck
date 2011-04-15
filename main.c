
#include <stdio.h>
#include <stdlib.h>

#include "lib/librobocheck.h"

int main(int argc, char **argv)
{
	int err_count;
	rbc_errset_t flags;
	struct rbc_input * input = (struct rbc_input *)malloc(sizeof(struct rbc_input));

	input->input_ptr = (struct rbc_static_input *)malloc(sizeof (struct rbc_static_input));
	input->tool_type = STATIC_TOOL;

	RESET(flags);
	
	// TODO:
	// this should go in init
	rbc_sys_entries[ERR_SPL_NULL_DEREF].data_type[0].xml_name = argv[argc-1];
	
	load_module(input, flags, &err_count, "/home/cezar/Desktop/rbc/modules/libsplint.so", "run_tool");	

	return 0;
}
