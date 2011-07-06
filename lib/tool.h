

#ifndef RBC_TOOL_H_
#define RBC_TOOL_H_

#include "rbc_api.h"

enum EN_tool_type
{
	INVALID_TOOL,
	STATIC_TOOL,
	DYNAMIC_TOOL
};

struct rbc_input
{
	const char **tool_args;
	int args_count;

	enum EN_tool_type tool_type;
	void *input_ptr;
};

struct rbc_output
{
	char *err_msg;
	enum EN_err_type err_type;

	struct rbc_out_info *aux_info;

	int size;
	struct rbc_output *next;
};


struct rbc_output *
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count);

/*
 * add
 *
 * Adds an error structure in the list of detected errors.
 *
 * returns: (nothing)
 * param1: list = the list of detected errors so far
 * param2: node = the data structure containing information
 * about the currently processed error
 */
inline void
add(struct rbc_output **list,struct rbc_output node)
{
	struct rbc_output *q=NULL,*p = *list;
	while (p!= NULL){//only adds if unique
		if (p->err_type == node.err_type &&
		    p->err_msg != NULL && node.err_msg != NULL &&
		    strcmp(p->err_msg,node.err_msg) == 0)
		{
			return;
		}
		q = p;
		p = p->next;
	}
	p = (struct rbc_output *) malloc(sizeof(struct rbc_output));
	p->err_type = node.err_type;
	p->err_msg = node.err_msg;
	p->next = NULL;

	if (q == NULL)
	{
		*list = p;
		(*list)->size = 1;
	}
	else
	{
		(*list)->size++;
		q->next = p;
	}
}

#endif
