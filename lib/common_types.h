
#ifndef RBC_TYPES_H_
#define RBC_TYPES_H_

#include "rbc_constants.h"
#include "rbc_errset.h"

enum EN_data_type
{
	RBC_NONE,
	
	RBC_INT,
	RBC_SHORT,
	RBC_LONG,
	RBC_FLOAT,
	RBC_DOUBLE,
	RBC_CHAR,
	RBC_PCHAR,
	
	RBC_TYPE_MAX
};

enum EN_rbc_access
{ 
	RBC_R, 
	RBC_W, 
	RBC_RW
};

typedef struct 
{
	enum EN_data_type type;
	int step;
	const char *err_msg, *err_penalty_msg;

	union
	{
		int int_value;
		short short_value;
		long long_value;
		float float_value;
		double double_value;
		char char_value;
		const char *str_value;
	} value;
} rbc_penalty_t;

typedef struct
{
	unsigned int bit_set[RBC_ERRSET_COUNT];
} rbc_errset_t;

typedef struct
{
	enum EN_rbc_access access;
	FILE *task_output;
} rbc_task_t;

struct rbc_out_info
{
	const char *msg;
	const char *penalty;
	float penalty_value;
};

#endif

