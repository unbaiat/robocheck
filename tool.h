

#ifndef RBC_TOOL_H_
#define RBC_TOOL_H_

#include "rbc_err_codes.h"

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
       
       enum EN_err_type err_type;
       void *aux;
};

struct rbc_output **
run_tool (struct rbc_input *, rbc_errset_t flags, int *err_count);

void *
parse (enum EN_err_type );

void *
fill_err_type  (enum EN_err_type );

#endif
