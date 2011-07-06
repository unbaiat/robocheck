#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/static_tool.h"

#define SPARSE_PATH	"./static_analyzer"

static void
make_command(char *command, struct rbc_input *input, struct rbc_static_input *static_input);

static struct rbc_output *
parse_output_file (FILE *f_in, rbc_errset_t flags, int file_count);

struct rbc_output *
run_tool(struct rbc_input *input, rbc_errset_t flags, int *err_count)
{
	int i = 0;
	char command[2 * MAX_BUFF_SIZE];
	FILE *f_in = NULL;
	struct rbc_static_input *static_input = NULL;
	struct rbc_output *output = NULL;

	if (input != NULL && input->tool_type == STATIC_TOOL && input->input_ptr != NULL)
	{
		static_input = (struct rbc_static_input *)input->input_ptr;
		if (static_input == NULL) { return NULL; }

		log_message("Hello from sparse", NULL);

		make_command(command, input, static_input);

		if (chdir("sparse-0.4.1/rbc_sparse_utils/"))
		{
			perror ("chdir");
			goto exit;
		}
		system (command);

		f_in = fopen("__sparse_output", "r");
		if (f_in == NULL)
		{
			perror("fopen");
			goto exit;
		}

		output = parse_output_file(f_in, flags, static_input->file_count);

		system ("rm -rf __sparse_output");
		if (chdir("../.."))
		{
			perror ("chdir");
		}
	}

exit:
	return output;
}

static void
make_command(char *command, struct rbc_input *input, struct rbc_static_input *static_input)
{
	int i = 0;

	sprintf(command, "%s", SPARSE_PATH); 
	for (i = 0; i < input->args_count; i++)
	{
		strcat (command, " ");
		strcat (command, input->tool_args[i]);
	}

	for (i = 0; i < static_input->file_count; i++)
	{
		strcat (command, " ");
		strcat (command, static_input->file_names[i]);
	}

	strcat (command, " > __sparse_output");
}

static struct rbc_output *
parse_output_file (FILE *f_in, rbc_errset_t flags, int file_count)
{
	int i;
	char buff[2 * MAX_BUFF_SIZE], file_name[2 * MAX_BUFF_SIZE], func_name[2 * MAX_BUFF_SIZE], *tmp = NULL, error_msg[4 * MAX_BUFF_SIZE];
	struct rbc_output *output = NULL, output_node;

	if (ISSET_ERR(ERR_UNVERIFIED_FUNCTION, flags))
	{
		while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
		{
			if (strstr(buff, "\t\tCall Errors")) { break; }
		}

		for (i = 0; i < file_count; i++)
		{
			/* get line with file name */
			fgets(buff, 2 * MAX_BUFF_SIZE, f_in);
			sscanf(buff, "####From file: %s####", file_name);
			tmp = strchr(file_name, '#'); if (tmp) { *tmp = 0; }

			/* parse all functions until the 'end file' is found */
			while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
			{
				if (strstr(buff, "#End parsing file#")) { break; }

				/* got the begining of a new function */
				sscanf(buff, "\t#####parsing function: %s#####", func_name);
				tmp = strchr(func_name, '#'); if (tmp) { *tmp = 0; }

				/* parse function summary until 'end function' is found */
				while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
				{
					if (strstr(buff, "#end parsing function#")) { break; }

					sprintf (error_msg, "From file: %s at function: %s error: %s", file_name, func_name, buff);
					output_node.err_msg = strdup(error_msg);
					output_node.err_type = ERR_UNVERIFIED_FUNCTION;

					add(&output, output_node);
				}
			}
		}
	}

	if (ISSET_ERR(ERR_FUNCTION_LINE, flags) ||
	    ISSET_ERR(ERR_FUNCTION_INDENTATION, flags) ||
	    ISSET_ERR(ERR_TRAILING_WHITESPACE, flags) ||
	    ISSET_ERR(ERR_MIXED_TABS, flags))
	{
		while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
		{
			if (strstr(buff, "Other errors")) { break; }
		}
		
		for (i = 0; i < file_count; i++)
		{
			/* get line with file name */
			fgets(buff, 2 * MAX_BUFF_SIZE, f_in);
			sscanf(buff, "####From file: %s####", file_name);
			tmp = strchr(file_name, '#'); if (tmp) { *tmp = 0; }
	
			/* parse all functions until the 'end file' is found */
			while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
			{
				if (strstr(buff, "#End parsing file#")) { break; }

				/* got the begining of a new function */
				sscanf(buff, "\t#####parsing function: %s#####", func_name);
				tmp = strchr(func_name, '#'); if (tmp) { *tmp = 0; }

				/* parse function summary until 'end function' is found */
				while (fgets(buff, 2 * MAX_BUFF_SIZE, f_in))
				{
					if (strstr(buff, "#end parsing function#")) { break; }

					if (ISSET_ERR(ERR_FUNCTION_LINE, flags) && 
					    strstr(buff, "Function line count:"))
					{
						sprintf (error_msg, "From file: %s at function: %s error: %s", file_name, func_name, buff);
						output_node.err_msg = strdup(error_msg);
						output_node.err_type = ERR_FUNCTION_LINE;

						add(&output, output_node);
					}

					if (ISSET_ERR(ERR_FUNCTION_INDENTATION, flags) && 
					    strstr(buff, "Indent level:"))
					{
						sprintf (error_msg, "From file: %s at function: %s error: %s", file_name, func_name, buff);
						output_node.err_msg = strdup(error_msg);
						output_node.err_type = ERR_FUNCTION_INDENTATION;

						add(&output, output_node);
					}

					if (ISSET_ERR(ERR_MIXED_TABS, flags) &&
					    strstr(buff, "Mixed tabs with spaces"))
					{
						sprintf (error_msg, "From file: %s at function: %s error: %s", file_name, func_name, buff);
						output_node.err_msg = strdup(error_msg);
						output_node.err_type = ERR_MIXED_TABS;

						add(&output, output_node);						
					}

					if (ISSET_ERR(ERR_TRAILING_WHITESPACE, flags) &&
					    strstr(buff, "Trailing white space"))
					{
						sprintf (error_msg, "From file: %s at function: %s error: %s.", file_name, func_name, buff);
						output_node.err_msg = strdup(error_msg);
						output_node.err_type = ERR_TRAILING_WHITESPACE;

						add(&output, output_node);						
					}
				}
			}
		}
	}

	return output;
}


