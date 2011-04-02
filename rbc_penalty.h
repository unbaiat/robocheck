
#ifndef RBC_PENALTY_H_
#define RBC_PENALTY_H_

#include "rbc_err_codes.h"

#define PENALTY_COUNT ERR_MAX

void
splint_null_deref_handler (void * );

void *penalty_handlers[PENALTY_COUNT] = 
{
       [ERR_SPL_NULL_DEREF] = splint_null_deref_handler
};

#endif
