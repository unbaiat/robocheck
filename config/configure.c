
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxml/xmlwriter.h>


#include "rbc_config.h"

void
help(void);

int main (int argc, char **argv)
{
    if (argc > 1)
    {
        rbc_xml_doc doc = xmlParseFile("rbc_config.xml");

        if (doc != NULL)
        {
            if (strcmp(argv[1], "--help") == 0)
            {
                help();
            }
            else if (strcmp(argv[1], "--list-all-tools") == 0)
            {
                list_all_tools(doc);
            }
            else if (strcmp(argv[1], "--list-startup-tools") == 0)
            {
                list_startup_tools(doc);
            }
            else if (strcmp(argv[1], "--list-tool-info") == 0)
            {
                if (argc > 2)
                {
                    list_tool_info(doc, argv[2]);
                }
                else
                {
                    printf ("Required tool name.\n");
                }
            }
            else if (strcmp(argv[1], "--list-err-info") == 0)
            {
                if (argc > 2)
                {
                    list_error_info(doc, argv[2]);
                }
                else
                {
                    printf ("Required error ID.\n");
                }
            }
            else if (strcmp(argv[1], "--list-all-errors") == 0)
            {
                list_errors(doc);
            }
            else if (strcmp(argv[1], "--create-tool") == 0)
            {
                if (argc > 4)
                {
                    create_tool(doc, argv[2], argv[3], argv[4]);
                }
                else
                {
                    printf ("Required: Tool Name, Tool Path, Tool type.\n");
                }
            }
            else if (strcmp(argv[1], "--register-tool") == 0)
            {
                if (argc > 2)
                {
                    register_tool(doc, argv[2]);
                }
                else
                {
                    printf ("Required: Tool Name.\n");
                }
            }
            else if (strcmp(argv[1], "--create-error") == 0)
            {
                if (argc > 2)
                {
                    //create_error(doc, argv[2]);
		    printf("Deprecated!\n");
                }
                else
                {
                    printf ("Required error description.\n");
                }
            }
            else if (strcmp(argv[1], "--register-error") == 0)
            {
                if (argc > 3)
                {
                    register_error(doc, argv[2], argv[3]);
                }
                else
                {
                    printf ("Required: error ID, tool name.\n");
                }
            }
            else if (strcmp(argv[1], "--register-parameter") == 0)
            {
                if (argc > 3)
                {
                    register_parameter(doc, argv[2], argv[3]);
                }
                else
                {
                    printf ("Required: tool running parameter,tool name.\n");
                }
            }
            else if (strcmp(argv[1], "--add-static-parameter") == 0)
            {
                if (argc > 2)
                {
                    add_static_parameter(doc, argv[2]);
                }
                else
                {
                    printf ("Required file source name.\n");
                }
            }
            else if (strcmp(argv[1], "--set-executable") == 0)
            {
                if (argc > 2)
                {
                    set_exec(doc, argv[2]);
                }
                else
                {
                    printf ("Required executable.\n");
                }
            }
            else if (strcmp(argv[1], "--add-dynamic-parameter") == 0)
            {
                if (argc > 2)
                {
                    add_dynamic_parameter(doc, argv[2]);
                }
                else
                {
                    printf ("Required parameter for executable.\n");
                }
            }
            else if (strcmp(argv[1], "--add-error-details") == 0)
            {
                if (argc > 6)
                {
                    add_err_details(doc, argv[2], argv[3], argv[4], argv[5], argv[6]);
                }
                else
                {
                    printf ("Required: error ID, penalty additional info, error count, penalty value, type.\n");
                }
            }
            else if (strcmp(argv[1], "--set-penalty-info") == 0)
            {
                if (argc > 3)
                {
                    set_libpenalty_info(doc, argv[2], argv[3]);
                }
                else
                {
                    printf ("Required: true/false, libpenalty path.\n");
                }
            }

             xmlSaveFormatFile ("rbc_config.xml", doc, 1);
            // xmlFreeDoc(doc);
            indent_xml_file("rbc_config.xml");
        }
        else
        {
            printf ("rbc_config.xml cannot be found.\n");
        }
    }
    else
    {
        printf ("Insufficient parameter count.");
    }

     // TEST AREA

     /*rbc_xml_doc doc = xmlParseFile("rbc_config.xml");

     char str[] = "Unitialized variable";
     create_error(doc, str);

    xmlSaveFormatFile ("rbc_config.xml", doc, 1);
    indent_xml_file("rbc_config.xml");*/
    
    return 0;
}

void
help(void)
{
    printf ("--list-all-tools\n");
    printf ("--list-startup-tools\n");
    printf ("--list-tool-info [tool name]\n");
    printf ("--list-err-info [error ID]\n");
    printf ("--list-all-errors\n");
    printf ("--create-tool [Tool Name] [Tool Path] [Tool type]\n");
    printf ("--register-tool [Tool Name]\n");
    printf ("--create-error [error description]\n");
    printf ("--register-error [error ID] [tool name]\n");
    printf ("--register-parameter [tool running parameter] [tool name]\n");
    printf ("--add-static-parameter [source name]\n");
    printf ("--set-executable [executable]\n");
    printf ("--add-dynamic-parameter [executable parameter]\n");
    printf ("--add-error-details [error ID] [penalty additional info] [error count] [penalty value] [penalty value type]\n");
    printf ("--set-penalty-info [true/false] [libpenalty path].\n");
}
