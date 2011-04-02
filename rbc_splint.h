
#ifndef RBC_SPLINT_H_
#define RBC_SPLINT_H_

#include "static_tool.h"
#include "rbc_spl_parse_utils.h"

#define SPL_ERR_COUNT (sizeof(detected_errs) / sizeof(detected_errs[0]))

enum EN_err_type detected_errs[] = 
{
	ERR_SPL_NULL_DEREF,
	ERR_SPL_UNDEF_STORAGE,
	ERR_SPL_TYPE_MISM,
	ERR_SPL_MEM_LEAK,
	ERR_SPL_ALIASING,
	ERR_SPL_GLOBALS,
	ERR_SPL_STATEMENT,
	ERR_SPL_BUFF_OF,
	ERR_SPL_NAMING
};

void *parse_functions[SPL_ERR_COUNT] = 
{
	[ERR_SPL_NULL_DEREF] = lookup_null_deref,
	[ERR_SPL_UNDEF_STORAGE] = lookup_undef_storage,
	[ERR_SPL_TYPE_MISM] = lookup_type_mismatch,
	[ERR_SPL_MEM_LEAK] = lookup_mem_leak,
	[ERR_SPL_ALIASING] = lookup_aliasing,
	[ERR_SPL_GLOBALS] = lookup_globals,
	[ERR_SPL_STATEMENT] = lookup_statement,
	[ERR_SPL_BUFF_OF] = lookup_buff_overflow,
	[ERR_SPL_NAMING] = lookup_naming
};

#endif
