#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define INT32_MAX 2147483647
#else
#include <stdint.h>
#endif

#include "../include/utils.h"
#include "../lib/penalty.h"

static rbc_penalty_t **__penalties = NULL;
static rbc_xml_doc __doc = NULL;

static void
create_err_mapping(rbc_xml_node );

static void
complete_err_info(rbc_penalty_t *, rbc_xml_node );

static int
fill_penalty_array(void);

static int
get_xml_err_count(void);

static enum EN_data_type
get_data_type (const char *type);

static void
set_rbc_value(rbc_penalty_t *, const char *type, const char *value);


int
init_penalties(rbc_xml_doc doc)
{
	int ret_status = -1, i;

	if (doc == NULL)
	{
		log_message("Invalid XML document openend.", NULL);
		goto exit;
	}

	__doc = doc;

	__penalties = (rbc_penalty_t **)malloc(PENALTY_COUNT * sizeof (** __penalties));
	if (__penalties == NULL)
	{
		log_message(NOMEM_ERR, NULL);
		goto exit;
	}

	for (i = 0; i < PENALTY_COUNT; i++)
	{
		__penalties[i] = NULL;
	}

	ret_status = fill_penalty_array();

exit:
	return ret_status;
}

void
free_penalties()
{
	int i;

	if (__doc == NULL) return;

	if (__penalties != NULL)
	{
		for (i = 0; i < PENALTY_COUNT; i++)
		{
			if (__penalties[i] != NULL)
			{
				free (__penalties[i]);
			}
		}

		free(__penalties);
	}
}

struct rbc_out_info *
apply_penalty(enum EN_err_type error_type, int count)
{
	int index = (int)error_type;
	struct rbc_out_info *ret_value = NULL;

	if (__doc == NULL || __penalties == NULL) goto exit;

	if (count > 0 &&
	    index >= 0 && index < PENALTY_COUNT)
	{
		ret_value = (struct rbc_out_info *) malloc(sizeof (*ret_value));
		if (ret_value == NULL)
		{
			log_message(NOMEM_ERR, NULL);
			goto exit;
		}

		ret_value->msg = __penalties[index]->err_msg; ret_value->penalty = __penalties[index]->err_penalty_msg;

		if (__penalties[index]->step == INT32_MAX)
		{
			ret_value->penalty_value = __penalties[index]->value.float_value;
		}
		else
		{
			count = count / __penalties[index]->step;
			ret_value->penalty_value = count * __penalties[index]->value.float_value;
		}
	}

exit:
	return ret_value;
}

static int
fill_penalty_array(void)
{
	int i, xml_err_count, ret_value = -1;

	rbc_xml_filter_t vec[] =
	{
		/* .filter = TAG_NAME, .filter_value.tag = "errors" */
		{TAG_NAME, "errors"}
	};

	rbc_xml_node node = NULL;

	if (__doc == NULL) goto exit;

	xml_err_count = get_xml_err_count();
	if (xml_err_count == -1) goto exit;

	node = lookup_node(__doc->children, vec, 1);
	if (node == NULL)
	{
		log_message("Invalid XML format. Cannot find any \"errors\" node.", NULL);
		goto exit;
	}

	ret_value = 0;
	node = get_next_node(get_child(node));
	for (i = 0; i < xml_err_count; i++)
	{
		if (node != NULL)
		{
			create_err_mapping(node);
		}
		node = get_next_node(node);
	}

exit:
	return ret_value;
}

static int
get_xml_err_count()
{
	int err_count = -1;
	const char *err_count_string = "";

	rbc_xml_filter_t vec[] =
	{
		/* .filter = TAG_NAME, .filter_value.tag = "init" */
		{TAG_NAME, "init"},
		/* .filter = TAG_NAME, .filter_value.tag = "err_count" */
		{TAG_NAME, "err_count"}
	};

	rbc_xml_node node = NULL;

	if (__doc == NULL) goto exit;

	node = lookup_node (__doc->children, vec, 2);
	if (node == NULL)
	{
		log_message("Invalid XML format. Cannot detect error count.", NULL);
		goto exit;
	}

	err_count_string = get_node_property(node, "value");
	if (err_count_string != NULL)
	{
		err_count = atoi(err_count_string);
	}

exit:
	return err_count;
}

static void
create_err_mapping(rbc_xml_node node)
{
	int err_id = 0, count = INT32_MAX;
	char *err_msg = NULL, *err_pen_msg = NULL;
	rbc_xml_node child_node = NULL;

	if (node != NULL)
	{
		child_node = get_next_node(get_child(node));
		if (child_node != NULL)
		{
			const char *err_id_str = "";
			const char *count_str = "";
			rbc_penalty_t *mapping = (rbc_penalty_t *) malloc(sizeof (*mapping));

			err_msg = (char *) get_node_property(node, "name");
			err_id_str = get_node_property(node, "id");
			if (err_id_str != NULL) { err_id = atoi(err_id_str); }

			err_pen_msg = (char *) get_node_property(child_node, "key");
			count_str = get_node_property(child_node, "count");
			if (count_str != NULL && strcmp(count_str, "INF") != 0) { count = atoi(count_str); }

			if (mapping != NULL)
			{
				mapping->step = count;
				mapping->err_msg = err_msg; mapping->err_penalty_msg = err_pen_msg;
				
				complete_err_info(mapping, child_node);

				__penalties[err_id] = mapping;
			}
			else
			{
				log_message(NOMEM_ERR, NULL);
			}
		}
	}
}

static void
complete_err_info(rbc_penalty_t *penalty, rbc_xml_node node)
{
	if (penalty != NULL && node != NULL)
	{
		const char *type_str = get_node_property(node, "type");
		const char *value_str = get_node_property(node, "value");

		set_rbc_value(penalty, type_str, value_str);
	}
}

static void
set_rbc_value (rbc_penalty_t *penalty, const char *type, const char *value)
{
	if (penalty != NULL &&
	    type != NULL && value != NULL)
	{
		penalty->type = get_data_type(type);

		switch (penalty->type)
		{
			case RBC_INT:
				sscanf(value, "%d", &(penalty->value.int_value));
				break;
			case RBC_DOUBLE:
			case RBC_FLOAT:
				sscanf(value, "%f", &(penalty->value.float_value));
				break;
			default:
				break;
		}
	}
}

static enum EN_data_type
get_data_type (const char *type)
{
	if (strcmp(type, "int") == 0)
	{
		return RBC_INT;
	}
	else if (strcmp(type, "short") == 0)
	{
		return RBC_SHORT;
	}
	else if (strcmp(type, "long") == 0)
	{
		return RBC_LONG;
	}
	else if (strcmp(type, "float") == 0)
	{
		return RBC_FLOAT;
	}
	else if (strcmp(type, "double") == 0)
	{
		return RBC_DOUBLE;
	}

	return RBC_NONE;
}

