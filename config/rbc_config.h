/* 
 * File:   rbc_config.h
 * Author: cezar
 *
 * Created on April 26, 2011, 11:55 PM
 */

#ifndef RBC_CONFIG_H
#define	RBC_CONFIG_H

#include "rbc_xml_parser.h"


void
list_all_tools(rbc_xml_doc );

void
list_startup_tools(rbc_xml_doc );

void
list_tool_info(rbc_xml_doc , const char * );

void
list_error_info(rbc_xml_doc , const char * );

void
list_errors(rbc_xml_doc );

int
create_tool(rbc_xml_doc doc, const char *tool_name,
            const char * tool_path, const char *tool_type);

int
register_tool(rbc_xml_doc doc, const char *tool_name);

int
create_error(rbc_xml_doc, char * );

int
register_error(rbc_xml_doc, const char *, const char * );

int
register_parameter(rbc_xml_doc, const char *, const char * );

int
inc_err_count (rbc_xml_doc );

int
add_static_parameter (rbc_xml_doc , const char * );

int
set_exec (rbc_xml_doc , const char * );

int
add_dynamic_parameter (rbc_xml_doc , const char * );

int
add_err_details (rbc_xml_doc ,
                 const char *err_id,
                 const char *description,
                 const char *count,
                 const char *value,
                 const char *type);

char *
emit_enum_string(char * );

char *
make_upper_case (char *);

int set_libpenalty_info (rbc_xml_doc doc, const char *load, const char *lib_path);

#endif	/* RBC_CONFIG_H */

