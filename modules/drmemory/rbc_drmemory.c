/*
 * rbc_drmemory.c: drmemory module
 *
 * General description:
 * 	Checks for invalid free/memory access, memory leaks, usage of
 * uninitialized variables and unclosed file descriptors. Parses the
 * output of drmemory according to the bit set of tracked errors
 * (user can choose to track a part, none or all types of errors).
 *
 * (C) 2012, Laura Vasilescu <laura@rosedu.org>
 *              * last review 20.06.2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/dynamic_tool.h"

#define LINE_MAX 512
#define DEFAULT_CMD "./drmemory -show_reachable -logdir . "
#define DETAILS "~~Dr.M~~ Details: "

#define SPACE " "
#define STDERR_TO_STDOUT " 2>&1 "
#define SEPARATORS "[]"

/*
 * Get function name.
 * Format:
 * '# NUMBER DYNAMIC_SOURCE!FUNCTION_NAME WHITESPACE*'
 */
static char *
get_function_name (char *name)
{
	char *ret = NULL, *p = NULL;
	
	p = strchr(name, '!');
	if (p == NULL)
		return NULL;

	ret = calloc(strlen(name), sizeof(char));
	strcpy(ret, p + 1);
	trim_whitespace(ret);

	return ret;
}

/*
 * Get filename from path.
 */
static const char *
get_simple_filename (const char *filename)
{
	const char *s = strrchr(filename, '/');
	if (s != NULL)
		s++;
	else
		s = filename;

	return s;
}

/*
 * Searches for a source name in a list of sources.
 */
static int
is_source (const char **sources, int source_count, char *source)
{
	const char *s;
	const char *ss = get_simple_filename(source);
	int i;

	if (sources != NULL) {
		for (i = 0; i < source_count; i++) {
			s = get_simple_filename(sources[i]);
			if (strcmp(ss, s) == 0)
				return 1;
		}
	}

	return 0;
}

/*
 * Extracts the three basic informations about an error:
 * - the function it apears in
 * - the source name
 * - the line number
 */
static int
parse_line (char *line, char **f_name, char **s_name, char **l_num,
	    struct rbc_dynamic_input *dynamic_input)
{
	char *first, *source;

	char *p = strtok(line, SEPARATORS);
	if (p == NULL)
		return 0;

	first = strdup(p);
	p = strtok(NULL, SEPARATORS);
	source = strdup(p);

	
	/* Get source filename. */
	*s_name = calloc(strlen(source), sizeof(char));
	p = strchr(source, ':');
	strncpy(*s_name, source, p - source);

	if (!is_source(dynamic_input->sources,
		      dynamic_input->source_count, *s_name)) {
		free(*s_name);
		free(source);
		free(first);
		return 0;
	}

	/* Get line number. */
	*l_num = calloc(strlen(source), sizeof(char));
	strncpy(*l_num, p + 1, strlen(source) - (long)(p - source) - 1);

	free(source);

	/* Get function name. */
	*f_name = get_function_name(first);

	free(first);

	return 1;
}


/*
 * Parses a line and extracts information about the
 * error (source, function, line, etc)
 */
static void
get_info (FILE *results, struct rbc_dynamic_input *dynamic_input,
	  struct rbc_output **output, enum EN_err_type err_type)
{
	char line[LINE_MAX];
	char error_msg[LINE_MAX];
	struct rbc_output node;
	char *f_name = NULL, *s_name = NULL, *l_num = NULL;

	memset(error_msg, 0, LINE_MAX);

	/* Get info for error. Output finishes with an empty line. */
	while (!feof(results)) {
		memset(line, 0, LINE_MAX);
		fgets(line, LINE_MAX, results);
		trim_whitespace(line);
		if (strlen(line) == 0)
			break;

		/* Check if it's a line from call-trace. */
		if (line[0] != '#')
			break;

		if (!parse_line(line, &f_name, &s_name, &l_num, dynamic_input))
			continue;

		strcat(error_msg, "In function ");
		strcat(error_msg, f_name);
		strcat(error_msg, ", in file ");
		strcat(error_msg, s_name);
		strcat(error_msg, ", at line ");
		strcat(error_msg, l_num);

		free(f_name);
		free(s_name);
		free(l_num);

		node.err_type = err_type;
		node.err_msg = strdup(error_msg);
		add(output, node);
	
		break;
	}
}

/*
 * Parse output from 'results' stream in order to extract all the errors
 * reported by drmemory.
 */
static void
parse_output (FILE *results, struct rbc_dynamic_input *dynamic_input,
	      rbc_errset_t flags, struct rbc_output **output)
{
	char line[LINE_MAX];

	while (!feof(results)) {
		memset(line, 0, LINE_MAX);
		fgets(line, LINE_MAX, results);
		fprintf(stderr, "&& %s", line);

		if (!strstr(line, "Error"))
			continue;

		if (ISSET_ERR(ERR_MEMORY_LEAK, flags)
		    && strstr(line, "LEAK")) {
			get_info(results, dynamic_input, output, ERR_MEMORY_LEAK);
			continue;
		}

		if (ISSET_ERR(ERR_INVALID_ACCESS, flags)
		    && strstr(line, "TODO")) {
			get_info(results, dynamic_input, output, ERR_INVALID_ACCESS);
			continue;
		}

		if (ISSET_ERR(ERR_UNINITIALIZED, flags)
		    && strstr(line, "TODO")) {
			get_info(results, dynamic_input, output, ERR_UNINITIALIZED);
			continue;
		}

		if (ISSET_ERR(ERR_INVALID_FREE, flags)
		    && strstr(line, "TODO")) {
			get_info(results, dynamic_input, output, ERR_INVALID_FREE);
			continue;
		}
	}
}

/*
 * Remove output directory created to log drmemory errors.
 */
static void remove_output_dir (char *name)
{
	char dir[LINE_MAX];
	char command[LINE_MAX];
	char *slash = NULL;

	memset(command, 0, LINE_MAX);
	memset(dir, 0, LINE_MAX);

	slash = strrchr(name, '/');
	strncpy(dir, name, slash - name);

	strcat(command, "rm -r ");
	strcat(command, dir);

	system(command);
}

/*
 * run_tool (every module contains this function)
 *
 * Runs a tool over the sources/executables and parses its output
 * in order to extract all the errors reported by the tool.
 */
struct rbc_output *
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count)
{
	char line[LINE_MAX], *name = NULL;
	char command[LINE_MAX] = DEFAULT_CMD;
	struct rbc_output *output = NULL;
	struct rbc_dynamic_input *dynamic_input = NULL;
	FILE *out_stream, *results;
	int i;

	if (input != NULL && input->input_ptr != NULL &&
	    input->tool_type == DYNAMIC_TOOL) {
		dynamic_input = (struct rbc_dynamic_input *)input->input_ptr;

		/* Add command line arguments. */
		for (i = 0; i < input->args_count; i++) {
			strcat(command, input->tool_args[i]);
			strcat(command, SPACE);
		}
		strcat(command, "-- ");
		
		strcat(command, dynamic_input->exec_name);
		strcat(command, STDERR_TO_STDOUT);

		/* Run tool. */
		out_stream = popen(command, "r");
		if (out_stream == NULL)
			return NULL;

		/* Get results file name. */
		while (!feof(out_stream)) {
			fgets(line, LINE_MAX, out_stream);
			if (strncmp(line, DETAILS, strlen(DETAILS)) == 0) {
				name = calloc(LINE_MAX, sizeof(char));
				strcpy(name, line + strlen(DETAILS));
				trim_whitespace(name);
				break;
			}
		}

		pclose(out_stream);
		/* TODO */
		sleep(1);
		
		if (name == NULL)
			return NULL;

		char *a = calloc(1000, 1);
		sprintf(a, "ls -l %s", name);
		system(a);

		/* Open output file. */
		results = fopen(name, "r");
		if (results == NULL) {
			free(name);
			return NULL;
		}

		/* Parse output and get errors. */
		parse_output(results, dynamic_input, flags, &output);

		fclose(results);

		/* Remove output directory. */
		remove_output_dir(name);

		free(name);
	}

	return output;
}
