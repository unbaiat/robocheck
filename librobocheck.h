
#ifndef LIB_RBC_H_
#define LIB_RBC_H_

#include "rbc_penalty.h"
#include "static_tool.h"
#include "dynamic_tool.h"

struct rbc_output **
load_module (struct rbc_input *, rbc_errset_t flags, int *err_count, char * libmodule, char *func_name);

void
test_func(enum EN_err_type );

#endif
