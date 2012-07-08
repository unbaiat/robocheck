
#ifndef RBC_UTILS_H_
#define RBC_UTILS_H_

#include "rbc_constants.h"

#define MAX(nr1, nr2) \
			((nr1 >= nr2) ? nr1 : nr2)

#define MIN(nr1, nr2) \
			((nr1 < nr2) ? nr1 : nr2)

extern char LoggerBuff[2 * MAX_BUFF_SIZE];
extern char CurrentModule[2 * MAX_BUFF_SIZE];

void
create_log_message (char * );

void
trim_whitespace (char *);

int
cmp_msg_file (char *m1, char *m2);

int
is_internal_stream (FILE *);

void
set_robocheck_module (void);

void
set_running_module (char *);

#endif
