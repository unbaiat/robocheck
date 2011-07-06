
#ifndef RBC_ERRSET_H_
#define RBC_ERRSET_H_

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
		do{\
			for (__errset_count = 0;\
			     __errset_count < RBC_ERRSET_COUNT;\
			     __errset_count++)\
			{\
				errset.bit_set[__errset_count] = 0;\
			}\
		} while(0)

#define SET_ERR(err_code, errset)\
		errset.bit_set[err_code / UINT_SIZE] |= 1 << (err_code % UINT_SIZE);


static unsigned int __errset_count = 0;

#endif

