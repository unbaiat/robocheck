
#ifndef RBC_TASK_H_
#define RBC_TASK_H_

#include "rbc_api.h"

rbc_task_t *
open_process (int argc, char **args, enum EN_rbc_access );

int
wait_process (rbc_task_t *);

#endif
