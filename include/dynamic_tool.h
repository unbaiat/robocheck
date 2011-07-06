
#ifndef RBC_DYNAMIC_TOOL_H_
#define RBC_DYNAMIC_TOOL_H_

#include "../lib/tool.h"

struct rbc_dynamic_input
{
	const char *exec_name;

	const char **params;
	int params_count;
	
	const char **sources;
	int source_count;
};

#endif
