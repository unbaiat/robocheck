
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "librobocheck.h"

extern void *penalty_handlers[PENALTY_COUNT];

struct rbc_output **
load_module(struct rbc_input *input, rbc_errset_t flags, int *err_count, char * libmodule, char *func_name)
{
	struct rbc_output **output = NULL;
	void *handle;
        struct rbc_output ** (* run_tool_ptr) (struct rbc_input *, rbc_errset_t flags, int *);
        char *error;

        handle = dlopen (libmodule, RTLD_LAZY);
        if (!handle) {
            fputs (dlerror(), stderr);
            exit(1);
        }

        run_tool_ptr = dlsym(handle, func_name);
        if ((error = dlerror()) != NULL)  {
            fputs(error, stderr);
            exit(1);
        }
	
	*err_count = 0;
	output = run_tool_ptr(input, flags, err_count);

        dlclose(handle);


	return output;
}

void
test_func(enum EN_err_type err_type)
{
	void (* f_ptr) (void *) = NULL;

	f_ptr = penalty_handlers[err_type];

	if (f_ptr != NULL)
	{
		f_ptr(NULL);
	}
}
