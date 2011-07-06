
#include <string.h>

#include "rbc_xml_parser.h"

static void *
function_handles[] = { [TAG_NAME] = check_node_name,
                       [PROPERTY_NAME] = check_node_property
                     };

rbc_xml_node
get_next_node (rbc_xml_node node)
{
    rbc_xml_node next_node = NULL;

    if (node != NULL)
    {
        next_node = node->next;
        if (next_node != NULL && next_node->type == XML_TEXT_NODE)
        {
            next_node = next_node->next;
        }
    }

    return next_node;
}

rbc_xml_node
get_child (rbc_xml_node node)
{
    rbc_xml_node child = NULL;

    if (node != NULL)
    {
        child = node->children;
    }

    return child;
}

void
list_nodes(rbc_xml_doc doc, rbc_xml_filter_t vec[], int count, 
           void (* display_f_ptr) (rbc_xml_node),
           void (* display_root_ptr) (rbc_xml_node))
{
    int index = 1;
    rbc_xml_node root = NULL;

    if (display_f_ptr != NULL &&
        (doc != NULL) &&
        (root = doc->children) != NULL)
    {
        root = lookup_node(root, vec, count);

        if (display_root_ptr != NULL)
        {
            display_root_ptr(root);
            printf ("\n");
        }

        root = get_next_node(get_child(root));

        while (root != NULL)
        {
            printf ("%d.\t", index++);
            display_f_ptr(root);
            root = get_next_node(root);
        }
    }
}

int
check_node_name (rbc_xml_node node, rbc_xml_filter_t filter)
{
    int ret_value = -1;

    if (node != NULL && 
        filter.filter_value.tag != NULL &&
        filter.filter == TAG_NAME)
    {
        ret_value = strcmp((char *)node->name, filter.filter_value.tag);
    }

    return ret_value;
}

int
check_node_property (rbc_xml_node node, rbc_xml_filter_t filter)
{
    int ret_value = -1;
    struct _xmlAttr* property_node = NULL;

    if (node != NULL &&
        filter.filter == PROPERTY_NAME &&
        filter.filter_value.property.name != NULL &&
        filter.filter_value.property.value != NULL)
    {
        if (node->properties != NULL)
        {
            property_node = node->properties;
            
            while (property_node != NULL)
            {
                if (property_node->name != NULL &&
                    strcmp((char *)property_node->name, filter.filter_value.property.name) == 0)
                {
                    if (property_node->children != NULL &&
                        property_node->children->content != NULL)
                    {
                        ret_value = strcmp((char *)property_node->children->content, filter.filter_value.property.value);
                        if (ret_value == 0)
                        {
                            break;
                        }
                    }
                }

                property_node = property_node->next;
            }
        }
    }

    return ret_value;
}

void
display_name (rbc_xml_node node)
{
    if (node != NULL)
    {
        printf ("%s\n", (char *)node->name);
    }
}

void
display_err_info(rbc_xml_node node)
{
    struct _xmlAttr* property_node = NULL;
    
    if (node != NULL && node->properties != NULL)
    {
        property_node = node->properties;

        // id
        if (property_node->name != NULL &&
            property_node->children != NULL &&
            property_node->children->content != NULL)
        {
            printf ("%s=%s\t", (char *)property_node->name,
                               (char *)property_node->children->content);
        }

        // error name
        property_node = property_node->next;
        if (property_node != NULL &&
            property_node->name != NULL &&
            property_node->children != NULL &&
            property_node->children->content != NULL)
        {
            printf ("%s=%s\n", (char *)property_node->name,
                               (char *)property_node->children->content);
        }
    }
}

void
display_first_attr(rbc_xml_node node)
{
    struct _xmlAttr* property_node = NULL;
    
    if (node != NULL && node->properties != NULL)
    {
        property_node = node->properties;

        if (property_node->name != NULL &&
            property_node->children != NULL &&
            property_node->children->content != NULL)
        {
            printf ("%s\n", (char *)property_node->children->content);
        }
    }
}

void
display_tool_info(rbc_xml_node node)
{
    rbc_xml_node temp_node = node;
    
    if (temp_node != NULL)
    {
        display_name(temp_node);

        temp_node = get_next_node(get_child(temp_node));

        while (temp_node != NULL)
        {
            printf ("\t\t");
            display_first_attr(temp_node);
            temp_node = get_next_node(temp_node);
        }
    }
}

void
display_err_header (rbc_xml_node node)
{
    const char *err_id = NULL, *err_name = NULL;
    struct _xmlAttr* property_node = NULL;

    if (node != NULL && node->properties != NULL)
    {
        property_node = node->properties;

        if (property_node->name != NULL &&
            property_node->children != NULL &&
            property_node->children->content != NULL)
        {
            err_id = (const char *)property_node->children->content;
        }

        property_node = property_node->next;
        if (property_node != NULL &&
            property_node->name != NULL &&
            property_node->children != NULL &&
            property_node->children->content != NULL)
        {
            err_name = (const char *)property_node->children->content;
        }

        printf ("\t\t Error name: %s - [id = %s]", err_name, err_id);
    }
}

void
display_err_sumary(rbc_xml_node node)
{
    rbc_xml_node temp_node = node;

    if (temp_node != NULL)
    {
        display_name(temp_node);

        temp_node = get_next_node(get_child(temp_node));

        while (temp_node != NULL)
        {
            printf ("\t\t");

            display_attr_list(temp_node);
            temp_node = get_next_node(temp_node);

            printf ("\n");
        }
    }
}

void
display_attr_list(rbc_xml_node node)
{
    if (node != NULL)
    {
        struct _xmlAttr* property_node = node->properties;

        while (property_node != NULL)
        {
            if (property_node != NULL &&
                property_node->name != NULL &&
                property_node->children != NULL &&
                property_node->children->content != NULL)
            {
                printf("%s - %s ",(const char *)property_node->name,
                                  (const char *)property_node->children->content);
            }

            property_node = property_node->next;
        }
    }
}

rbc_xml_node
lookup_node(rbc_xml_node root, rbc_xml_filter_t path[], int count)
{
    int i = 0;
    rbc_xml_node node = root;

    for (i = 0; i < count; i++)
    {
        node = get_next_node(get_child(node));
        //if (path[i].filter_value.tag != NULL)
        {
            while (node != NULL)
            {
                int (* f_ptr) (rbc_xml_node, rbc_xml_filter_t) = function_handles[path[i].filter];
                if (f_ptr != NULL && f_ptr(node, path[i]) == 0)
                {
                    break;
                }
                node = get_next_node(node);
            }
        }

        if (node == NULL)
        {
            break;
        }
    }

    return node;
}

int
indent_xml_file (const char * f_name)
{
    int status_code = -1;
    rbc_xml_doc doc = NULL;

    if (f_name != NULL)
    {
         xmlKeepBlanksDefault(0);
         doc = xmlParseFile(f_name);
         
         if (doc != NULL)
         {
             xmlSaveFormatFile (f_name, doc, 1);
             xmlFreeDoc(doc);

             status_code = 0;
         }
    }

    return status_code;
}

const char *
get_node_property(rbc_xml_node node, const char *name)
{
    if (name != NULL &&
        node != NULL && node->properties != NULL)
    {
        xmlAttr *cursor = node->properties;
        while (cursor != NULL)
        {
            if (cursor != NULL &&
                strcmp((char *)cursor->name, name) == 0)
            {
                return (cursor->children != NULL) ?
                       (const char *)cursor->children->content :
                       NULL;
            }
            cursor = cursor->next;
        }
    }

    return NULL;
}

int
set_node_property_value(rbc_xml_node node, const char *property, const char *value)
{
    int err_status = -1;

    if (property != NULL && value != NULL &&
        node != NULL && node->properties != NULL)
    {
        xmlAttr *cursor = node->properties;
        while (cursor != NULL)
        {
            if (cursor->name != NULL &&
                strcmp((char *)cursor->name, property) == 0)
            {
                cursor->children->content = (xmlChar*)value;
                err_status = 0;
                break;
            }
            cursor = cursor->next;
        }
    }

    return err_status;
}