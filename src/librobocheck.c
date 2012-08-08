#define _CRT_SECURE_NO_WARNINGS 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include "../wrapper/dlfcn.h"
#else
	#include <dlfcn.h>
#endif

#include "../include/utils.h"
#include "../include/librobocheck.h"


extern rbc_xml_doc __root;
extern struct rbc_dynamic_input *__dynamic_ptr;
extern struct rbc_static_input *__static_ptr;
extern int __rbc_err_count;

static void * __libpenalty = NULL;
static struct rbc_out_info * (* apply_penalty_ptr) (enum EN_err_type , int );

static int __output_size = 0, __output_inc_count = 0;
static struct rbc_output **__output = NULL;


#ifdef RBC_DEBUG
static void
print_vector ();
#endif

static void
update_errors(void);

static void
sort_output_vector(void);

static void
free_output_vector(void);

static struct rbc_output *
dup_rbc_output (struct rbc_output *);

static int
check_libpenalty(char **);

static void
add_range(struct rbc_output * );

void
close_robocheck (void)
{
	int ret_value = 0;
	
	close_libpenalty();

	ret_value = is_internal_stream (FileLogger);
	if (ret_value != 0)
	{
		fclose (FileLogger);
	}

	if (__dynamic_ptr != NULL)
	{
		if (__dynamic_ptr->params != NULL)
		{
			free (__dynamic_ptr->params);
		}
		free (__dynamic_ptr);
	}
	if (__static_ptr != NULL)
	{
		if (__static_ptr->file_names != NULL)
		{
			free (__static_ptr->file_names);
		}
		free (__static_ptr);
	}

	log_message("Freed static and dinamic input", NULL);

	if (__root != NULL)
	{
		xmlFreeDoc(__root);
	}

	free_output_vector();
}

int
init_robocheck (FILE *logger, FILE *out)
{
	FileLogger = logger;
	OutputStream = out;
	
	read_startup_info();

	set_robocheck_module();

	load_libpenalty();

	return 0;
}

struct rbc_static_input *
extract_static_input (rbc_xml_doc doc_ptr)
{
	int i;

	if (__static_ptr == NULL && doc_ptr != NULL) /* singleton */
	{
		rbc_xml_node node;
		const char *f_count = "";
		rbc_xml_filter_t vec[] = {
			/* .filter = TAG_NAME, .filter_value.tag = "init"  */
			{TAG_NAME, "init"},
			/* .filter = TAG_NAME, .filter_value.tag = "input" */
			{TAG_NAME, "input"},
			/* .filter = TAG_NAME, .filter_value.tag = "static" */
			{TAG_NAME, "static"}
		};

		__static_ptr = (struct rbc_static_input *)malloc(sizeof(struct rbc_static_input));
		if (__static_ptr == NULL)
		{
			log_message("Insufficient memory alocating 'static input'.\n", NULL);
			goto exit;
		}

		__static_ptr->file_names = NULL;

		node = lookup_node(__root->children, vec, 3);
		if (node == NULL)
		{
			log_message("Invalid format for XML config file.\n", NULL);
			goto exit;
		}

		/* get file count */
		f_count = get_node_property(node, "file_count");
		__static_ptr->file_count = (f_count != NULL) ? atoi(f_count) : 0;

		/* get file names (if any available) */
		if (__static_ptr->file_count > 0)
		{
			__static_ptr->file_names = (const char **)malloc(__static_ptr->file_count  * sizeof (const char *));
			if (__static_ptr->file_names == NULL)
			{
				log_message("Insufficient memory for tested file sources.\n", NULL);
				goto exit;
			}

			node = get_next_node(get_child(node));
			for (i = 0; i < __static_ptr->file_count; i++)
			{
				__static_ptr->file_names[i] = get_node_property(node, "value");
				node = get_next_node(node);
			}
		}
	}

exit:
	return __static_ptr;
}

int
extract_error_count(rbc_xml_doc doc_ptr)
{
	if (doc_ptr != NULL && __rbc_err_count < 0)
	{
		rbc_xml_filter_t vec[] = {
			/* .filter = TAG_NAME, .filter_value.tag = "init" */
			{TAG_NAME, "init"},
			/* .filter = TAG_NAME, .filter_value.tag = "err_count" */
			{TAG_NAME, "err_count"}
		};
		const char *value = "";

		rbc_xml_node node = lookup_node(__root->children, vec, 2);
		if (node == NULL)
		{
			log_message("Invalid format for XML config file.\n", NULL);
			goto exit;
		}

		value = get_node_property(node, "value");
		if (value != NULL)
		{
			__rbc_err_count = atoi(value);
		}
	}

exit:
	return __rbc_err_count;
}

struct rbc_dynamic_input *
extract_dynamic_input (rbc_xml_doc doc_ptr)
{
	int i;

	if (__dynamic_ptr == NULL && doc_ptr != NULL) /* singleton */
	{
		rbc_xml_filter_t vec[] = {
			/* .filter = TAG_NAME, .filter_value.tag = "init" */
			{TAG_NAME, "init"},
			/* .filter = TAG_NAME, .filter_value.tag = "input" */
			{TAG_NAME, "input"},
			/* .filter = TAG_NAME, .filter_value.tag = "dynamic" */
			{TAG_NAME, "dynamic"}
		};
		rbc_xml_node node;
		const char *argc = "";

		__dynamic_ptr = (struct rbc_dynamic_input *)malloc(sizeof(struct rbc_dynamic_input));
		if (__dynamic_ptr == NULL)
		{
			log_message("Insufficient memory alocating 'dynamic input'.\n", NULL);
			goto exit;
		}

		__dynamic_ptr->params = NULL;

		node = lookup_node(__root->children, vec, 3);
		if (node == NULL)
		{
			log_message("Invalid format for XML config file.\n", NULL);
			goto exit;
		}

		/* get executable name */
		__dynamic_ptr->exec_name = get_node_property(node, "value");

		/* set auxiliary information */
		if (__static_ptr != NULL)
		{
			__dynamic_ptr->sources = __static_ptr->file_names;
			__dynamic_ptr->source_count = __static_ptr->file_count;
		}

		/* get argc */
		argc = get_node_property(node, "arg_count");
		__dynamic_ptr->params_count = (argc != NULL) ? atoi(argc) : 0;

		/* get argv (if any available) */
		if (__dynamic_ptr->params_count > 0)
		{
			__dynamic_ptr->params = (const char **)malloc(__dynamic_ptr->params_count  * sizeof (const char *));
			if (__dynamic_ptr->params == NULL)
			{
				log_message("Insufficient memory for tested program arguments.\n", NULL);
				goto exit;
			}

			node = get_next_node(get_child(node));
			for (i = 0; i < __dynamic_ptr->params_count; i++)
			{
				__dynamic_ptr->params[i] = get_node_property(node, "value");
				node = get_next_node(node);
			}			
		}
	}

exit:
	return __dynamic_ptr;
}

rbc_errset_t
extract_tool_errset (rbc_xml_doc doc_ptr, const char * tool_name)
{
	char buff[256] = {0};
	int i = 0, tool_err_count = 0, err_id;
	rbc_errset_t tool_errs;
	rbc_xml_node tool_node = NULL, err_node = NULL;

	if (doc_ptr != NULL && tool_name)
	{
		rbc_xml_filter_t vec[] = {
			/* .filter = TAG_NAME, .filter_value.tag = "installed_tools" */
			{TAG_NAME, "installed_tools"},
			/* .filter = TAG_NAME, .filter_value.tag = tool_name */
			{TAG_NAME, ""},
			/* .filter = TAG_NAME, .filter_value.tag = "errors" */
			{TAG_NAME, "errors"}
		};
		const char *err_count_string = "";

		vec[1].filter_value.tag = tool_name;

		RESET(tool_errs);
		
		tool_node = lookup_node(__root->children, vec, 3);
		if (tool_node == NULL)
		{
			sprintf(buff, "Invalid XML config format. No '%s' tool installed.\n", tool_name);
			log_message(buff, NULL);
			goto exit;
		}

		err_count_string = get_node_property(tool_node, "err_count");
		tool_err_count = (err_count_string != NULL) ? atoi(err_count_string) : 0;

		err_node = get_next_node(get_child(tool_node));
		for (i = 0; i < tool_err_count; i++)
		{
			const char *err_id_string = get_node_property(err_node, "value");
			err_id = (err_id_string != NULL) ? atoi(err_id_string) : 0;

			if (err_id != 0)
			{
				SET_ERR(err_id, tool_errs);
			}
		
			err_node = get_next_node(err_node);
		}
	}

exit:
	return tool_errs;
}

struct rbc_output *
load_module(struct rbc_input *input, rbc_errset_t flags, int *err_count, const char * libmodule, const char *func_name)
{
        char *error, buff[1024] = {0}, *tool_name = NULL;
	void *handle;
	struct rbc_output *output = NULL;
        struct rbc_output * (* run_tool_ptr) (struct rbc_input *, rbc_errset_t flags, int *);

	tool_name = strrchr(libmodule, '/');
	tool_name = (tool_name != NULL) ? tool_name + 1 : (char *)libmodule;
	sprintf(buff, "Attempting to run tool: '%s'", tool_name);
	log_message(buff, NULL);

        handle = dlopen (libmodule, RTLD_LAZY);
        if (!handle)
	{
		log_message (dlerror(), NULL);

		fprintf(stderr, "Failed loading module %s.\n", libmodule);
		goto exit_function;
        }

        run_tool_ptr = dlsym(handle, func_name);
        if ((error = dlerror()) != NULL)
	{
		log_message (error, NULL);

		fprintf(stderr, "Failed loading symbol %s from module %s.\n", func_name, libmodule);
		goto exit_function;
        }
	
	*err_count = 0;
	if (run_tool_ptr != NULL)
	{
		set_running_module(tool_name);
		output = run_tool_ptr(input, flags, err_count);
		set_robocheck_module();
	}

        dlclose(handle);

exit_function:
	if (output == NULL)
	{
		sprintf(buff, "Running shared object '%s' returned NULL output.\n", libmodule);
		log_message(buff, NULL);
	}

	return output;
}

void read_startup_info(void)
{
	__root = xmlParseFile("rbc_config.xml");
	if (__root ==  NULL)
	{
		log_message("Error opening config file.\n", NULL);
		return;
	}

	extract_error_count(__root);
	extract_static_input(__root);
	extract_dynamic_input(__root);
}

void
run_robocheck()
{
	int i, tool_count = 0, err_count;
	rbc_xml_node tool_node = NULL, current_tool_node = NULL;
	struct rbc_output *output = NULL;
	const char *tool_count_str = "";

	rbc_xml_filter_t vec[] =
	{
		/* .filter = TAG_NAME, .filter_value.tag = "init" */
		{TAG_NAME, "init"},
		/* .filter = TAG_NAME, .filter_value.tag = "tools" */
		{TAG_NAME, "tools"}
	};

	tool_node = lookup_node(__root->children, vec, 2);
	if (tool_node == NULL)
	{
		log_message("Invalid XML format", NULL);
		return;
	}

	/* extract tool count */
	tool_count_str = get_node_property(tool_node, "count");
	tool_count = (tool_count_str != NULL) ? atoi(tool_count_str) : 0; 

	/* for each registered tool */
	current_tool_node = get_next_node(get_child(tool_node));
	for (i = 0; i < tool_count; i++)
	{
		const char *tool_name = get_node_property(current_tool_node, "value");

		rbc_xml_filter_t vec[] =
		{
			/* .filter = TAG_NAME, .filter_value.tag = "installed_tools" */
			{TAG_NAME, "installed_tools"},
			/* .filter = TAG_NAME, .filter_value.tag = tool_name */
			{TAG_NAME, ""}
		};				
		const char *lib_path = "";
		const char *type = "";
		rbc_errset_t errset;
		enum EN_tool_type tool_type;
		struct rbc_input *input = NULL;
		rbc_xml_node current_tool;

		vec[1].filter_value.tag = tool_name;

		if (tool_name == NULL) {
			continue;
		}

		current_tool = lookup_node(__root->children, vec, 2);
		if (current_tool == NULL)
		{
			continue;
		}

		lib_path = get_node_property(current_tool, "lib_path");
		type = get_node_property(current_tool, "type");		

		errset = extract_tool_errset(__root, tool_name);

		tool_type = get_type(type);
		input = extract_tool_input(tool_name, tool_type);

		output = load_module(input, errset, &err_count, lib_path, "run_tool");
		add_range(output);

		current_tool_node = get_next_node(current_tool_node);
	}

	update_errors();
}

enum EN_tool_type
get_type(const char *type)
{
	enum EN_tool_type tool_type = INVALID_TOOL;

	if (type != NULL)
	{
		if (strcmp(type, "dynamic") == 0)
		{
			tool_type = DYNAMIC_TOOL;
		}
		else if (strcmp(type, "static") == 0)
		{
			tool_type = STATIC_TOOL;
		}
	}

	return tool_type;
}

struct rbc_input *
extract_tool_input(const char *tool_name, enum EN_tool_type tool_type)
{
	char buff[256] = {0};
	int i, param_count;
	struct rbc_input *input = NULL;
	rbc_xml_node param_node = NULL, current_param_node = NULL;
	rbc_xml_filter_t vec[] =
	{
		/* .filter = TAG_NAME, .filter_value.tag = "installed_tools" */
		{TAG_NAME, "installed_tools"},
		/* .filter = TAG_NAME, .filter_value.tag = tool_name  */
		{TAG_NAME, ""},
		/* .filter = TAG_NAME, .filter_value.tag = "parameters" */
		{TAG_NAME, "parameters"}
	};
	const char *param_count_str = "";
	vec[1].filter_value.tag = tool_name;

	input = (struct rbc_input *) malloc(sizeof(struct rbc_input));
	if (input == NULL)
	{
		log_message("Insufficient memory for alocating input pointer\n", NULL);
		goto exit;
	}

	input->tool_type = tool_type;
	if (tool_type == DYNAMIC_TOOL)
	{
		input->input_ptr = __dynamic_ptr;
	}
	else if (tool_type == STATIC_TOOL)
	{
		input->input_ptr = __static_ptr;
	}
	
	param_node = lookup_node(__root->children, vec, 3);
	if (param_node == NULL)
	{
		sprintf(buff, "Invalid XML format for tool %s.\n", tool_name);
		log_message(buff, NULL);
		goto exit;
	}

	param_count_str = get_node_property(param_node, "param_count");
	param_count = (param_count_str != NULL) ? atoi(param_count_str) : 0;

	input->tool_args = (const char **) malloc(sizeof(const char *) * param_count);
	if (input->tool_args == NULL)
	{
		input->args_count = 0;
		log_message("Insufficient memory alocating parameter vector.\n", NULL);
		goto exit;
	}

	input->args_count = param_count;

	current_param_node = get_next_node(get_child(param_node));	
	for (i = 0; i < param_count; i++)
	{
		const char *param_string = get_node_property(current_param_node, "value");
		input->tool_args[i] = param_string;

		current_param_node = get_next_node(current_param_node);
	}

exit:
	return input;
}

void
close_libpenalty(void)
{
	char *error = NULL;
	void (* free_penalties_ptr)(void);

	if (__libpenalty == NULL) { return ; }

	free_penalties_ptr = dlsym(__libpenalty, "free_penalties");
	if ((error = dlerror()) != NULL)
	{
		log_message (error, NULL);
		return;
	}

	if (free_penalties_ptr != NULL) { free_penalties_ptr(); }

	dlclose(__libpenalty);
	log_message("Penalty module unloaded succesfully", NULL);
}

int
load_libpenalty(void)
{
	int ret_status = -1, open_status = -1;
	char *lib_path = NULL, *error = NULL;
	int (* fptr_init) (rbc_xml_doc );

	open_status = check_libpenalty(&lib_path);

	log_message("Attempting to load penalty module", NULL);

	if (open_status == 0 && lib_path != NULL)
	{
		__libpenalty = dlopen(lib_path, RTLD_LAZY);
		if (!__libpenalty)
		{
			log_message (dlerror(), NULL);
			goto exit;
		}

		fptr_init = dlsym(__libpenalty, "init_penalties");
		if ((error = dlerror()) != NULL)
		{
			log_message (error, NULL);
			goto exit;
		}
		
		if (fptr_init != NULL && fptr_init(__root) == 0)
		{	
			ret_status = 0;
			apply_penalty_ptr = dlsym(__libpenalty, "apply_penalty");
			if ((error = dlerror()) != NULL)
			{
				log_message (error, NULL);
				goto exit;
			}	
		}
	}

exit:
	if (ret_status != 0)
	{
		log_message("Penalty module not loaded.", NULL);
	}
	else
	{
		log_message("Penalty module loaded succesfully", NULL);
	}

	return ret_status;
}

static int
check_libpenalty(char **lib_path)
{
	int ret_value = -1;
	rbc_xml_node node = NULL;
	rbc_xml_filter_t vec[] =
	{
		/* .filter = TAG_NAME, .filter_value.tag = "init"  */
		{TAG_NAME, "init"},
		/* .filter = TAG_NAME, .filter_value.tag = "penalty" */
		{TAG_NAME, "penalty"}
	};
	const char *load = "";

	if (__root == NULL) goto exit;

	node = lookup_node(__root->children, vec, 2);
	if (node == NULL)
	{
		log_message("Invalid XML format. "
			    "Cannot extract details regarding the penalty module.",
			    NULL);
		goto exit;
	}

	ret_value = 0;
	load = get_node_property(node, "load");
	if (strcmp(load, "true") == 0)
	{
		*lib_path = (char *) get_node_property(node, "lib_path");
	}

exit:
	return ret_value;
}

/*
 * Check for duplicates reported errors. (from different modules)
 */
static int
check_for_duplicates (struct rbc_output *output)
{
	int i;

	for (i = 0; i < __output_size; i++) {
		if (output->err_type == __output[i]->err_type 
		    && output->err_msg != NULL && __output[i]->err_msg != NULL
		    && cmp_msg_file(output->err_msg, __output[i]->err_msg))
			return 1;
	}

	return 0;
}

static void
add_range(struct rbc_output *output)
{
	struct rbc_output *crs = NULL, *tmp = NULL;

	if (__output == NULL)
	{
		__output = (struct rbc_output **) malloc(ALLOC_INC * sizeof (struct rbc_output *));
		if (__output == NULL)
		{
			log_message(NOMEM_ERR, NULL);
			return;
		}

		__output_inc_count = 1;
	}

	if (output != NULL && output->size > 0)
	{
		crs = output;
		while (crs != NULL)
		{
			if ((__output_size + 1) % ALLOC_INC == 0)
			{
				struct rbc_output **temp_output = NULL;
				__output_inc_count++;

				temp_output = (struct rbc_output **) realloc (__output, __output_inc_count * sizeof(struct rbc_output *));
				if (temp_output != NULL)
				{
					__output = temp_output;
				}
				else
				{
					free (__output); __output = NULL;
					log_message(NOMEM_ERR, NULL);
					return;
				}
			}

			tmp = crs;
			if (!check_for_duplicates(crs))
				__output[__output_size++] = dup_rbc_output(crs);
			crs = crs->next;
			free (tmp);
		}
	}
}

static struct rbc_output *
dup_rbc_output (struct rbc_output *output_node)
{
	struct rbc_output *ret_node = NULL;

	if (output_node == NULL) { goto exit; }

	ret_node = (struct rbc_output *) malloc(sizeof (struct rbc_output));
	if (ret_node == NULL)
	{
		log_message(NOMEM_ERR, NULL);
		goto exit;
	}

	ret_node->err_msg = output_node->err_msg;
	ret_node->err_type = output_node->err_type;

	ret_node->aux_info = NULL;

exit:
	return ret_node;
}

static void
free_output_vector()
{
	int i = 0, start = 0;

	if (__output != NULL)
	{
		start = 0;
		for (i = 0; i < __output_size; i++)
		{
			if (__output[i] != NULL)
			{
				/* FIXME: */
				/*if (__output[i]->aux_info != NULL && 
				    __output[start]->aux_info != NULL
				    __output[i]->err_type != __output[start]->err_type)
				{
					free (__output[start]->aux_info);
					__output[start]->aux_info = NULL;
					start = i;
				}*/

				free (__output[i]); __output[i] = NULL;
			}
		}

		__output_size = __output_inc_count = 0;
		free (__output); __output = NULL;
	}
}

static void
sort_output_vector ()
{
	int i,j;

	if (__output == NULL) { return; }

	for (i = 0; i < __output_size; i++)
	{
		for (j = i + 1; j < __output_size; j++)
		{
			if (__output[i] != NULL && __output[j] != NULL &&
			    __output[i]->err_type > __output[j]->err_type)
			{
				struct rbc_output *temp = __output[i];
				__output[i] = __output[j];
				__output[j] = temp;
			}
		}
	}
}

static void
json_output_start()
{
	fprintf(OutputStream, "{\n\t\"result\":\n\t[\n");
	fflush(OutputStream);
}

static void
json_output_error(struct rbc_out_info *info)
{
	fprintf(OutputStream, "\t\t{\n");
	fprintf(OutputStream, "\t\t\t\"name\" : \"%s\",\n", info->msg);
	fprintf(OutputStream, "\t\t\t\"key\" : \"%s\",\n", info->penalty);
	fprintf(OutputStream, "\t\t\t\"value\" : \"%.2f\",\n", info->penalty_value);
	fprintf(OutputStream, "\t\t\t\"where\" :\n\t\t\t[\n");
	fflush(OutputStream);
}

static void
json_output_error_message(char *msg, int code)
{
	msg = strdup(msg);
	trim_whitespace(msg);
	fprintf(OutputStream, "\t\t\t\t{ \"line\" : \"%s\" }", msg);
	free(msg);

	if (!code)
		fprintf(OutputStream, ",");
	fprintf(OutputStream, "\n");
	fflush(OutputStream);
}

static void
json_output_error_end(int code)
{
	fprintf(OutputStream, "\t\t\t]\n\t\t}");
	if (!code)
		fprintf(OutputStream, ",");
	fprintf(OutputStream, "\n");
	fflush(OutputStream);
}

static void
json_output_end()
{
	fprintf(OutputStream, "\t]\n}\n");
	fflush(OutputStream);
}

static void
update_errors()
{
	int i, j, start, count;
	struct rbc_out_info *aux = NULL;
	static char penalty_buff[MAX_BUFF_SIZE];

	if (__libpenalty == NULL || apply_penalty_ptr == NULL || __output == NULL) { return; }

#ifdef RBC_DEBUG
	print_vector();
#endif
	sort_output_vector ();
#ifdef RBC_DEBUG
	print_vector();
#endif

	log_message("Penalty results: ", NULL);
	json_output_start();

	start = 0;
	for (i = 0; i <= __output_size; i++)
	{
		if (i == __output_size || (__output[i]->err_type != __output[start]->err_type))
		{
#ifdef RBC_DEBUG
			printf ("[%d- %d]\n", start, i - 1);
#endif

			count = (i - 1) - start + 1;
			
			aux = apply_penalty_ptr(__output[start]->err_type, count);
			if (aux != NULL)
			{
				sprintf (penalty_buff, "- %.2f p  | %s | %s", aux->penalty_value, aux->msg, aux->penalty);
				log_message(penalty_buff, NULL);
				json_output_error(aux);
				

				for (j = start; j <= (i - 1); j++)
				{
					sprintf (penalty_buff, "\t\t%s", __output[j]->err_msg);
					log_message(penalty_buff, NULL);
					json_output_error_message(__output[j]->err_msg, j == (i - 1));

					__output[j]->aux_info = aux;
				}

				json_output_error_end(i == __output_size);
			}

			start = i;
		}
	}

	json_output_end();
}

#ifdef RBC_DEBUG
static void
print_vector ()
{
	int i;

	for (i = 0; i < __output_size; i++)
	{
		printf ("%d ", __output[i]->err_type);
	}
	printf ("\n");
}
#endif

