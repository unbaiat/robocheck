
#ifndef RBC_UTILS_H_
#define RBC_UTILS_H_

#include "common_types.h"
#include "rbc_task.h"

#define MAX(nr1, nr2) \
			((nr1 >= nr2) ? nr1 : nr2)

#define MIN(nr1, nr2) \
			((nr1 < nr2) ? nr1 : nr2)


void
create_log_message (char * );

void
close_robocheck (void);

int
init_robocheck (void);

int
is_internal_stream (FILE *);

int
log_message (char *, FILE *);

void
rbc_free_mem (void **mem_ptr);

void *
rbc_get_mem (size_t count, size_t element_size);

char *
make_comm_string (int argc, char **args);

#endif
