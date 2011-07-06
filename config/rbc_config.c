
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "rbc_config.h"

static char str_number[10];
static char __str[15] = "err_";

void
list_all_tools(rbc_xml_doc doc)
{
    rbc_xml_filter_t vec[] =
    {
        { .filter = TAG_NAME, .filter_value.tag = "installed_tools" }
    };

    printf ("\t\t Installed tools:\n");
    list_nodes(doc, vec, 1, display_name, NULL);
}

void
list_startup_tools(rbc_xml_doc doc)
{
    rbc_xml_filter_t vec[] =
    {
        { .filter = TAG_NAME, .filter_value.tag = "init" },
        { .filter = TAG_NAME, .filter_value.tag = "tools" }
    };

    printf ("\t\t Robocheck startup tools:\n");
    list_nodes(doc, vec, 2, display_first_attr, NULL);
}

void
list_errors(rbc_xml_doc doc)
{
    rbc_xml_filter_t vec[] =
    {
        { .filter = TAG_NAME, .filter_value.tag = "errors" }
    };

    printf ("\t\t Registered errors:\n");
    list_nodes(doc, vec, 1, display_err_info, NULL);
}

void
list_tool_info(rbc_xml_doc doc, const char * tool_name)
{
    if (tool_name != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "installed_tools" },
            { .filter = TAG_NAME, .filter_value.tag = tool_name }
        };

        printf ("\t\t Tool %s:\n", tool_name);
        printf ("Path:\t");
        list_nodes(doc, vec, 2, display_tool_info, display_first_attr);
    }
}

void
list_error_info(rbc_xml_doc doc, const char * err_str_id)
{
    if (err_str_id != NULL)
    {
        rbc_xml_property_t prop =
        {
            .name = "id",
            .value = err_str_id
        };

        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "errors" },
            { .filter = PROPERTY_NAME, .filter_value.property = prop }
        };

        list_nodes(doc, vec, 2, display_err_sumary, display_err_header);
    }
}

int
create_tool(rbc_xml_doc doc, const char *tool_name, 
            const char * tool_path, const char *tool_type)
{
    int status_code = -1;
    rbc_xml_node root = NULL, new_node = NULL, param_node = NULL, err_node;

    if (doc != NULL && doc->children != NULL &&
        tool_name != NULL && tool_path != NULL && tool_type != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "installed_tools" }
        };

       root = lookup_node(doc->children, vec, 1);

       new_node = xmlNewTextChild (root, NULL, (xmlChar *) tool_name, NULL);
       xmlNewProp (new_node, (xmlChar *) "lib_path", (xmlChar *) tool_path);
       xmlNewProp (new_node, (xmlChar *) "type", (xmlChar *) tool_type);

       const char *count_string = get_node_property(root, "count");
       if (count_string != NULL)
       {
           int count = atoi(count_string); count++;
           sprintf(__str, "%d", count);
           set_node_property_value(root, "count", __str);
       }
       
       param_node = xmlNewTextChild (new_node, NULL, (xmlChar *) "parameters", NULL);
       xmlNewProp (param_node, (xmlChar *) "param_count", (xmlChar *) "0");

       err_node = xmlNewTextChild (new_node, NULL, (xmlChar *) "errors", NULL);
       xmlNewProp (err_node, (xmlChar *) "err_count", (xmlChar *) "0");

       new_node = xmlNewTextChild (new_node, NULL, (xmlChar *) "input", NULL);
       xmlNewTextChild (new_node, NULL, (xmlChar *) tool_type, NULL);

       status_code = 0;
    }

    return status_code;
}

int
register_error(rbc_xml_doc doc, const char *err_id, const char * tool_name)
{
    int status_code = -1;
    rbc_xml_node err_node = NULL, node = NULL;

    if (doc != NULL && doc->children != NULL &&
        tool_name != NULL && err_id != NULL)
    {
        rbc_xml_property_t prop =
        {
            .name = "value",
            .value = err_id
        };

        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "installed_tools" },
            { .filter = TAG_NAME, .filter_value.tag = tool_name },
            { .filter = TAG_NAME, .filter_value.tag = "errors" },
            { .filter = PROPERTY_NAME, .filter_value.property = prop }
        };

        err_node = lookup_node(doc->children, vec, 3);
        // TODO:
        node = lookup_node(doc->children, vec, 4);

        if (node == NULL)
        {
            status_code = 0;
            node = xmlNewTextChild (err_node, NULL, (xmlChar *) "add", NULL);
            xmlNewProp (node, (xmlChar *) "value", (xmlChar *) err_id);

            const char *value = get_node_property(err_node, "err_count");
            if (value != NULL)
            {
                int err_count = atoi(value); err_count++;
                sprintf(__str, "%d", err_count);
                set_node_property_value(err_node, "err_count", __str);
            }
        }
        else
        {
            printf ("Error with id=%s allready registered for tool=%s\n", err_id, tool_name);
        }
    }

    return status_code;
}

int
register_parameter(rbc_xml_doc doc, const char *tool_parameter, const char * tool_name)
{

    int status_code = -1;
    rbc_xml_node param_node = NULL, node = NULL;

    if (doc != NULL && doc->children != NULL &&
        tool_name != NULL && tool_parameter != NULL)
    {
        rbc_xml_property_t prop =
        {
            .name = "value",
            .value = tool_parameter
        };

        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "installed_tools" },
            { .filter = TAG_NAME, .filter_value.tag = tool_name },
            { .filter = TAG_NAME, .filter_value.tag = "parameters" },
            { .filter = PROPERTY_NAME, .filter_value.property = prop }
        };

        param_node = lookup_node(doc->children, vec, 3);
        // TODO:
        node = lookup_node(doc->children, vec, 4);

        if (node == NULL)
        {
            status_code = 0;
            
            node = xmlNewTextChild (param_node, NULL, (xmlChar *) "add", NULL);
            xmlNewProp (node, (xmlChar *) "value", (xmlChar *) tool_parameter);

            const char *value = get_node_property(param_node, "param_count");
            if (value != NULL)
            {
                int params_count = atoi(value);
                params_count++; sprintf(__str, "%d", params_count);
                set_node_property_value(param_node, "param_count", __str);
            }
        }
        else
        {
            printf ("Parameter %s allready registered for tool=%s\n", tool_parameter, tool_name);
        }
    }

    return status_code;
}

int
inc_err_count (rbc_xml_doc doc)
{
    int err_count = 0;
    const char *str_err_count = NULL;

    struct _xmlAttr* property_node = NULL;
    rbc_xml_node node = NULL;
    
    if (doc != NULL && doc->children != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "init" },
            { .filter = TAG_NAME, .filter_value.tag =  "err_count" }
        };

        node = lookup_node(doc->children, vec, 2);
        if (node != NULL && node->properties != NULL)
        {
            property_node = node->properties;

            if (property_node->name != NULL &&
                property_node->children != NULL &&
                property_node->children->content != NULL)
            {
                str_err_count = (const char *)property_node->children->content;
                if (str_err_count != NULL)
                {
                    err_count = atoi(str_err_count);
                    err_count++; sprintf(str_number, "%d", err_count);

                    property_node->children->content = (xmlChar *)str_number;
                }
            }
        }
    }

    return err_count;
}

int
create_error(rbc_xml_doc doc, char * err_description)
{
    rbc_xml_node node = NULL;
    char buff[1024] = {0}, *enum_err = NULL, *tmp;

    if (doc != NULL && err_description != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "errors" }            
        };

        node = lookup_node(doc->children, vec, 1);
        
        if (node != NULL)
        {
            inc_err_count(doc);
            strcat(__str, str_number);

            node = xmlNewTextChild (node, NULL, (xmlChar *) __str, NULL);
            xmlNewProp (node, (xmlChar *) "id", (xmlChar *) str_number);
            xmlNewProp (node, (xmlChar *) "name", (xmlChar *) err_description);

            tmp = strdup(err_description);
            enum_err = emit_enum_string(err_description);

            sprintf(buff, "bash add_line.sh ERR_MAX %s \"%s\" lib/rbc_err_codes.h", enum_err, tmp);
            system(buff);
        }
    }

    return 0;
}

int
add_static_parameter (rbc_xml_doc doc, const char * file_name)
{
    int status_code = -1;
    rbc_xml_node input_node = NULL, static_node = NULL, new_node = NULL;

    if (doc != NULL && file_name != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "init" },
            { .filter = TAG_NAME, .filter_value.tag = "input" },
            { .filter = TAG_NAME, .filter_value.tag = "static"}
        };

        input_node = lookup_node(doc->children, vec, 2);
        static_node = lookup_node(doc->children, vec, 3);

        if (input_node != NULL)
        {
            if (static_node == NULL)
            {
                static_node = xmlNewTextChild (input_node, NULL, (xmlChar *) "static", NULL);
                xmlNewProp (static_node, (xmlChar *) "file_count", (xmlChar *) "0");
            }

            new_node = xmlNewTextChild (static_node, NULL, (xmlChar *) "add", NULL);
            xmlNewProp (new_node, (xmlChar *) "value", (xmlChar *) file_name);

            const char *file_count = get_node_property(static_node, "file_count");
            if (file_count != NULL)
            {
                int file_count_val = atoi(file_count);
                file_count_val++; sprintf(__str, "%d", file_count_val);
                set_node_property_value(static_node, "file_count", __str);
            }

            status_code = 0;
        }
    }

    return status_code;
}

int
set_exec (rbc_xml_doc doc, const char * exec_name)
{
    int status_code = -1;
    rbc_xml_node input_node = NULL, dynamic_node = NULL;
    struct _xmlAttr* property_node = NULL;

    if (doc != NULL && exec_name != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "init" },
            { .filter = TAG_NAME, .filter_value.tag = "input" },
            { .filter = TAG_NAME, .filter_value.tag = "dynamic"}
        };

        input_node = lookup_node(doc->children, vec, 2);
        dynamic_node = lookup_node(doc->children, vec, 3);

        if (input_node != NULL)
        {
            if (dynamic_node == NULL)
            {
                dynamic_node = xmlNewTextChild (input_node, NULL, (xmlChar *) "dynamic", NULL);
                xmlNewProp (dynamic_node, (xmlChar *) "value", (xmlChar *) exec_name);
                xmlNewProp (dynamic_node, (xmlChar *) "arg_count", (xmlChar *) "0");
            }
            else if (dynamic_node != NULL)
            {
                if (dynamic_node->properties != NULL)
                {
                    property_node = dynamic_node->properties;

                    if (property_node->name != NULL &&
                        property_node->children != NULL &&
                        property_node->children->content != NULL)
                    {
                        property_node->children->content = (xmlChar *) exec_name;
                    }
                }
                else
                {
                    xmlNewProp (dynamic_node, (xmlChar *) "value", (xmlChar *) exec_name);
                }
            }

            status_code = 0;
        }
    }

    return status_code;
}

int
add_dynamic_parameter (rbc_xml_doc doc, const char * parameter)
{
    int status_code = -1;
    rbc_xml_node dynamic_node = NULL, temp_node = NULL;

    if (doc != NULL && parameter != NULL)
    {
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "init" },
            { .filter = TAG_NAME, .filter_value.tag = "input" },
            { .filter = TAG_NAME, .filter_value.tag = "dynamic"}
        };

        dynamic_node = lookup_node(doc->children, vec, 3);

        if (dynamic_node != NULL)
        {
            temp_node = xmlNewTextChild (dynamic_node, NULL, (xmlChar *) "add", NULL);
            xmlNewProp (temp_node, (xmlChar *) "value", (xmlChar *) parameter);

            /* increment value the number of executable parameters */
            const char *arg_count = get_node_property(dynamic_node, "arg_count");
            if (arg_count != NULL)
            {
                int argc = atoi(arg_count);
                sprintf(__str, "%d", (argc + 1));
                set_node_property_value(dynamic_node, "arg_count", __str);
            }

            status_code = 0;
        }
    }

    return status_code;
}

int
add_err_details (rbc_xml_doc doc,
                 const char *err_id,
                 const char *description,
                 const char *count,
                 const char *value,
                 const char *type)
{
    int status_code = -1;
    rbc_xml_node err_node = NULL, child_node = NULL;

    if (doc != NULL && err_id != NULL &&
        description != NULL &&
        value != NULL && count != NULL &&
        type != NULL)
    {
        rbc_xml_property_t prop =
        {
            .name = "id",
            .value = err_id
        };

        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "errors" },
            { .filter = PROPERTY_NAME, .filter_value.property = prop }
        };

        err_node = lookup_node(doc->children, vec, 2);

        if (err_node != NULL)
        {
            child_node = get_next_node(get_child(err_node));
            if (child_node == NULL)
            {
                err_node = xmlNewTextChild (err_node, NULL, (xmlChar *) "add", NULL);
                xmlNewProp (err_node, (xmlChar *) "key", (xmlChar *) description);
                xmlNewProp (err_node, (xmlChar *) "count", (xmlChar *) count);
                xmlNewProp (err_node, (xmlChar *) "value", (xmlChar *) value);
                xmlNewProp (err_node, (xmlChar *) "type", (xmlChar *) type);
            }
            else
            {
                set_node_property_value(child_node, "key", description);
                set_node_property_value(child_node, "count", count);
                set_node_property_value(child_node, "value", value);
                set_node_property_value(child_node, "type", type);
            }

            status_code = 0;
        }
    }

    return status_code;
}

int
register_tool(rbc_xml_doc doc, const char *tool_name)
{
    int status_code = -1;
    rbc_xml_node node = NULL, new_node = NULL;

    if (doc != NULL && tool_name != NULL)
    {
        /* check if the tool is installed */
        rbc_xml_filter_t vec[] =
        {
            { .filter = TAG_NAME, .filter_value.tag = "installed_tools" },
            { .filter = TAG_NAME, .filter_value.tag = tool_name }
        };

        node = lookup_node(doc->children, vec, 2);
        if (node == NULL)
        {
            fprintf(stderr, "Given tool is not installed: %s.\n", tool_name);
            goto exit;
        }

        /* tool is installed */
        /* now register the tool */
        /* first find the correct position in the XML */
        rbc_xml_filter_t init_lookup[] =
        {
            {.filter = TAG_NAME, .filter_value.tag = "init"},
            {.filter = TAG_NAME, .filter_value.tag = "tools"}
        };

        node = lookup_node(doc->children, init_lookup, 2);
        if (node == NULL)
        {
            fprintf(stderr, "Unexpected format for XML config file.\n");
            goto exit;
        }

        /* add the new node to the XML */
         new_node = xmlNewTextChild (node, NULL, (xmlChar *) "add", NULL);
         xmlNewProp (new_node, (xmlChar *) "value", (xmlChar *) tool_name);

         const char *count_string = get_node_property(node, "count");
         if (count_string != NULL)
         {
             int count = atoi(count_string); count++;
             sprintf(__str, "%d", count);
             set_node_property_value(node, "count", __str);
         }

         status_code = 0;
    }

exit:
    return status_code;
}

char *
emit_enum_string (char *string)
{
    char *upper_string = NULL;
    char *token1 = NULL, *token2 = NULL;

    if (string == NULL)
    {
        goto exit;
    }

    upper_string = (char *)malloc(strlen(string) + 1);
    if (upper_string == NULL)
    {
        goto exit;
    }
    memset(upper_string, 0, strlen(upper_string) + 1);
    
    token1 = strtok(string, " ");
    if (token1 != NULL)
    {
        token1 = make_upper_case(token1);
        if (token1 != NULL)
        {
            sprintf(upper_string, "ERR_%s", token1);
            free(token1);
        }
        
        token2  = strtok(NULL, " ");
        if (token2 != NULL)
        {
            token2 = make_upper_case(token2);
            if (token2 != NULL)
            {
                sprintf(upper_string + strlen(upper_string), "_%s", token2);
                #ifdef RBC_DEBUG
                free(token2);
                #endif
            }
        }
    }
    
exit:
    return upper_string;
}

char *
make_upper_case (char * string)
{
    int len = 0;
    char *ret_string = NULL;

    if (string != NULL)
    {
        len = strlen(string);
        ret_string = (char *)malloc(len + 1);
        if (ret_string != NULL)
        {
            memset(ret_string, 0, len + 1);
            while (*string != '\0')
            {
                *ret_string++ = toupper(*string++);
            }
        }

        ret_string -= len;
    }

    return ret_string;
}

int set_libpenalty_info (rbc_xml_doc doc, const char *load, const char *lib_path)
{
    int ret_value = -1;
    rbc_xml_node node = NULL;

    if (doc != NULL &&
        load != NULL && lib_path != NULL)
    {
        if (strcmp(load, "true") == 0 || strcmp(load, "false") == 0)
        {
            rbc_xml_filter_t vec[] =
            {
                {.filter = TAG_NAME, .filter_value.tag = "init"},
                {.filter = TAG_NAME, .filter_value.tag = "penalty"}
            };

            node = lookup_node(doc->children, vec, 2);
            if (node == NULL)
            {
                fprintf(stderr, "Invalid XML format.\n");
                goto exit;
            }

            ret_value = 0;

            set_node_property_value(node, "load", load);
            set_node_property_value(node, "lib_path", lib_path);
        }
        else
        {
            fprintf(stderr, "Invalid penalty option. Try true or false.\n");
        }
    }

exit:
    return ret_value;
}