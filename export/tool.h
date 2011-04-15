

#ifndef RBC_TOOL_H_
#define RBC_TOOL_H_

#include "../lib/common_types.h"
#include "../lib/rbc_err_codes.h"

#define GET_RBC_ERR_ENTRY(err_code, index) \
			rbc_sys_entries[err_code].data_type[index]

enum EN_tool_type
{
     STATIC_TOOL,
     DYNAMIC_TOOL
};

struct rbc_input
{
       char **tool_args;
       
       enum EN_tool_type tool_type;
       void *input_ptr;
};

struct rbc_output
{
	char *file_name, *function_name;
	int line_number;
	char *err_msg;
       
	struct rbc_out_info *aux_info;
};

extern __rbc_entry_t rbc_sys_entries[PENALTY_COUNT];

struct rbc_output **
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count);

void
apply_penalty (struct rbc_out_info *);

#endif
