
#ifndef RBC_ERR_CODES_H_
#define RBC_ERR_CODES_H_

#define BYTE_SIZE 8
#define RBC_ERRSET_COUNT (ERR_MAX / UINT_SIZE + 1)
#define UINT_SIZE (sizeof (unsigned int) * BYTE_SIZE)


#define CLR_ERR(err_code, errset)\
		errset.bit_set[err_code / UINT_SIZE] &= ~(1 << (err_code % UINT_SIZE));

#define ISSET_ERR(err_code, errset)\
		(\
			(\
				errset.bit_set[err_code / UINT_SIZE] & \
				(1 << (err_code % UINT_SIZE))\
			 )\
				!= 0\
		)

#define RESET(errset) \
		for (__errset_count = 0; \
		     __errset_count < RBC_ERRSET_COUNT; \
		     __errset_count++) \
		{\
			errset.bit_set[__errset_count] = 0;\
		}

#define SET_ERR(err_code, errset)\
		errset.bit_set[err_code / UINT_SIZE] |= 1 << (err_code % UINT_SIZE);


enum EN_err_type
{
	ERR_SPL_NULL_DEREF,
	ERR_SPL_UNDEF_STORAGE,
	ERR_SPL_TYPE_MISM,
	ERR_SPL_MEM_LEAK,
	ERR_SPL_ALIASING,
	ERR_SPL_GLOBALS,
	ERR_SPL_STATEMENT,
	ERR_SPL_BUFF_OF,
	ERR_SPL_NAMING,
	
	ERR_MAX
};

struct __rbc_err
{
	unsigned int bit_set[RBC_ERRSET_COUNT];
};

typedef struct __rbc_err rbc_errset_t;

static unsigned int __errset_count = 0;

#endif

