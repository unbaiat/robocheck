
#ifndef RBC_SPL_PARSE_H_
#define RBC_SPL_PARSE_H_

void
lookup_null_deref(void);
void
lookup_undef_storage(void);
void
lookup_type_mismatch(void);
void
lookup_mem_leak(void);
void
lookup_aliasing(void);
void
lookup_globals(void);
void
lookup_statement(void);
void
lookup_buff_overflow(void);
void
lookup_naming(void);

#endif

