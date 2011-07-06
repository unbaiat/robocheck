/* 
 * File:   rbc_xml_parser.h
 * Author: cezar
 *
 * Created on March 17, 2011, 12:53 AM
 */

#ifndef RBC_XML_PARSER_H
#define	RBC_XML_PARSER_H

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>

#define VISITED 0xff

#ifdef  __XML_TREE_H__
typedef xmlNode * rbc_xml_node;
typedef xmlDocPtr rbc_xml_doc;
#else
typedef void* rbc_xml_node;
typedef void* rbc_xml_doc;
#endif

enum EN_xml_lookup
{
    TAG_NAME,
    PROPERTY_NAME
};

typedef struct
{
    const char *name;
    const char *value;
} rbc_xml_property_t;

typedef struct
{
    enum EN_xml_lookup filter;
    
    union filter_value_t
    {
        const char *tag;
        rbc_xml_property_t property;
    } filter_value;
} rbc_xml_filter_t;


rbc_xml_doc
open_file (const char *fname);

rbc_xml_node
get_next_node (rbc_xml_node );

rbc_xml_node
get_child(rbc_xml_node );

void
list_nodes(rbc_xml_doc, rbc_xml_filter_t[], int, 
           void (* display_f_ptr) (rbc_xml_node),
           void (* display_root_ptr) (rbc_xml_node));

int
check_node_name (rbc_xml_node, rbc_xml_filter_t);

int
check_node_property (rbc_xml_node, rbc_xml_filter_t);

void
display_name (rbc_xml_node);

void
display_err_info(rbc_xml_node );

void
display_first_attr(rbc_xml_node );

void
display_tool_info(rbc_xml_node );

void
display_err_header(rbc_xml_node );

void
display_err_sumary(rbc_xml_node );

void
display_attr_list(rbc_xml_node );

rbc_xml_node
lookup_node(rbc_xml_node, rbc_xml_filter_t [], int );

int
indent_xml_file (const char * );

const char *
get_node_property(rbc_xml_node node, const char *name);

int
set_node_property_value(rbc_xml_node node, const char *property, const char *value);

#endif	/* RBC_XML_PARSER_H */

