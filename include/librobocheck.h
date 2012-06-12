
#ifndef LIB_RBC_H_
#define LIB_RBC_H_

#include "static_tool.h"
#include "dynamic_tool.h"

#include "../lib/penalty.h"
#include "../lib/rbc_utils.h"

rbc_xml_doc __root = NULL;
struct rbc_dynamic_input *__dynamic_ptr = NULL;
struct rbc_static_input *__static_ptr = NULL;
int __rbc_err_count = -1;

FILE * FileLogger = NULL;
FILE * OutputStream = NULL;
char LoggerBuff[2 * MAX_BUFF_SIZE];
char CurrentModule[2 * MAX_BUFF_SIZE];


void
close_robocheck (void);

int
init_robocheck (FILE *, FILE *);

int
extract_error_count(rbc_xml_doc doc_ptr);

struct rbc_static_input *
extract_static_input (rbc_xml_doc doc_ptr);

struct rbc_dynamic_input *
extract_dynamic_input (rbc_xml_doc doc_ptr);

rbc_errset_t
extract_tool_errset (rbc_xml_doc doc_ptr, const char * tool_name);

struct rbc_output *
load_module (struct rbc_input *, rbc_errset_t flags, int *err_count, const char * libmodule, const char *func_name);

void
read_startup_info(void);

void
run_robocheck(void);

struct rbc_input *
extract_tool_input(const char *tool_name, enum EN_tool_type tool_type);

enum EN_tool_type
get_type(const char *type);

void
close_libpenalty(void);

int
load_libpenalty(void);

#endif
