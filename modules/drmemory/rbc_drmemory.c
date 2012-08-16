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
 *              * last review 09.08.2012
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#ifndef popen
		#define popen _popen
		#define pclose _pclose
	#endif
#endif

#include "../../include/dynamic_tool.h"
#include "../../include/utils.h"

#define LINE_MAX 512
#ifdef _WIN32
	#define DEFAULT_CMD "drmemory.exe -show_reachable -redzone_size 0 -logdir  . "
#else
	#define DEFAULT_CMD "drmemory -show_reachable -redzone_size 0 -logdir . "
#endif
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
	char *ret = NULL, *p = NULL, *a;
	
	p = strchr(name, '!');
	if (p == NULL) {
		a = strdup(name);
		/* # */
		p = strtok(a, " \t");
		if (p == NULL)
			return NULL;
		/* number */
		p = strtok(NULL, " \t");
		if (p == NULL)
			return NULL;
		/* name */
		p = strtok(NULL, " \t");
		if (p == NULL)
			return NULL;
		
		ret = calloc(strlen(name), sizeof(char));
		strcpy(ret, p);
		trim_whitespace(ret);
		free(a);
		
		return ret;
	}

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

	const char *s = NULL;

#ifdef _WIN32
	s = strrchr(filename, '\\');
#else
	s = strrchr(filename, '/');
#endif

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
parse_line (char *lline, char **f_name, char **s_name, char **l_num,
	    struct rbc_dynamic_input *dynamic_input)
{
	char *first, *source, *p;
	char *line = strdup(lline);
	int len;

	if (line == NULL || strlen(line) == 0)
		return 0;

	p = strtok(line, SEPARATORS);
	if (p == NULL)
		return 0;

	first = strdup(p);
	p = strtok(NULL, SEPARATORS);
	if (p == NULL) {
		free(first);
		return 0;
	}
	source = strdup(p);

	/* Get source filename. */
	len = strlen(source);
	*s_name = strdup(source);
	p = strrchr(source, ':');
	(*s_name)[p - source] = '\0';

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
	free(line);

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
	char line[LINE_MAX + 1];
	char error_msg[LINE_MAX];
	struct rbc_output node;
	char *f_name = NULL, *s_name = NULL, *l_num = NULL;

	memset(error_msg, 0, LINE_MAX);

	/* Get info for error. Output finishes with an empty line. */
	while (!feof(results)) {
		memset(line, 0, LINE_MAX + 1);
		fgets(line, LINE_MAX, results);
		trim_whitespace(line);
		if (strlen(line) == 0)
			break;

		/* Check if it's a line from call-trace. */
		if (line[0] != '#')
			break;

		if (!parse_line(line, &f_name, &s_name, &l_num, dynamic_input))
			continue;

		sprintf(error_msg, "In function %s, in file %s, at line %s",
			f_name, s_name, l_num);

		free(f_name);
		free(s_name);
		free(l_num);

		node.err_type = err_type;
		node.err_msg = calloc(strlen(error_msg) + 1, sizeof(char));
		strcpy(node.err_msg, error_msg);
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

		if (!strstr(line, "Error"))
			continue;

		if (ISSET_ERR(ERR_MEMORY_LEAK, flags)
		    && strstr(line, "LEAK")) {
			get_info(results, dynamic_input, output, ERR_MEMORY_LEAK);
			continue;
		}

		if (ISSET_ERR(ERR_INVALID_ACCESS, flags)
		    && strstr(line, "UNADDRESSABLE ACCESS")) {
			get_info(results, dynamic_input, output, ERR_INVALID_ACCESS);
			continue;
		}

		if (ISSET_ERR(ERR_UNINITIALIZED, flags)
		    && strstr(line, "UNINITIALIZED READ")) {
			get_info(results, dynamic_input, output, ERR_UNINITIALIZED);
			continue;
		}

		if (ISSET_ERR(ERR_INVALID_FREE, flags)
		    && strstr(line, "INVALID HEAP ARGUMENT") && strstr(line, "free")) {
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

#ifdef _WIN32
	/* Close opened notepad */
	system("taskkill /f /im notepad.exe >NUL 2>NUL");
	slash = strrchr(name, '\\');
#else
	slash = strrchr(name, '/');
#endif
	strncpy(dir, name, slash - name);

#ifdef _WIN32
	strcat(command, "rmdir /q /s ");
#else
	strcat(command, "rm -r ");
#endif
	strcat(command, dir);

	system(command);
}

static void
wait_end_of_writing (char *filename)
{
#ifndef _WIN32
	FILE *in = NULL;
	char line[LINE_MAX];
	char command[LINE_MAX];

	memset(command, 0, LINE_MAX);
	sprintf(command, "fuser %s 2> /dev/null", filename);

	while (1) {
		in = popen(command, "r");

		memset(line, 0, LINE_MAX);
		fgets(line, LINE_MAX, in);
		trim_whitespace(line);

		pclose(in);

		if (strlen(line) == 0)
			break;
	}
#endif
}

void modify_name_path(char *str)
{
	#ifdef _WIN32
	int i;
	
	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '/') 
			str[i] = '\\';
	}
	
	#endif
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

		if (name == NULL)
			return NULL;
		modify_name_path(name);

		wait_end_of_writing(name);
		pclose(out_stream);

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
