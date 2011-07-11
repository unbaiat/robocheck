
#ifndef RBC_ERR_CODES_H_
#define RBC_ERR_CODES_H_

enum EN_err_type
{
	ERR_NONE,

	ERR_MEMORY_LEAK,		// Memory leak

	ERR_INVALID_ACCESS,		// Invalid access exception

	ERR_UNINITIALIZED,		// Uninitialized

	ERR_FILE_DESCRIPTORS,		// File descriptors error

	ERR_DATA_RACE,		// Data race

	ERR_DEAD_LOCK,		// Dead lock

	ERR_UNLOCK,		// Unlock

	ERR_DESTROY,		// Destroy

	ERR_CONDITION_VARIABLE,		// Condition variable

	ERR_HOLD_LOCK,		// Hold lock

	ERR_DUPLICATE_CODE,		// Duplicate code

	ERR_STATIC_VARIABLE,		// static variable

	ERR_SIGNED_UNSIGNED,		// signed unsigned

	ERR_UNVERIFIED_FUNCTION,		// Unverified function return call

	ERR_FUNCTION_LINE,		// Function line count exceeds maximum admited

	ERR_FUNCTION_INDENTATION,		// Function indentation exceeds maximum admited

	ERR_TRAILING_WHITESPACE,		// Trailing whitespace

	ERR_MIXED_TABS,		// Mixed tabs with spaces

	ERR_INVALID_FREE,		// invalid free

	ERR_MAX
};

#endif


