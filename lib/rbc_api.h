
#ifndef LIB_RBC_API_H_
#define LIB_RBC_API_H_

#include "common_types.h"
#include "rbc_task.h"
#include "../config/rbc_config.h"

int
log_message (char *, FILE *);

void
rbc_free_mem (void **mem_ptr);

void *
rbc_get_mem (size_t count, size_t element_size);

char *
make_comm_string (int argc, char **args);

#endif

