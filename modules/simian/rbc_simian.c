/*
 * rbc_simian.c: simian module
 *
 * General description: 
 * 	Checks for duplicate code.
 *	Parses the output of simian.
 * 	Simian supports many programming languages.
 *	Threshold (number of lines to be considered as duplicate)
 * can be adjusted.
 *
 * (C) 2011, Iulia Bolcu <reea_mod@yahoo.com>
 * (C) 2012, Laura Vasilescu <laura@rosedu.org>
 *              * last review 09.08.2012
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/static_tool.h"

#define LINE_MAX 512
#define MSG_SIZE 2048
#define DEFAULT_CMD "java -jar /lib/simian-2.3.32.jar"
#define OUTPUT " > output"
#define SPACE " "
#define OUTPUT_FILE "output"
#define RM_OUTPUT "rm output"

/*
 * print_list
 *
 * Prints the list of errors. Auxiliary function, used
 * for debugging.
 *
 * returns: (nothing)
 * param1: list = the list of errors
 */

static void
print_list(struct rbc_output *list){
	struct rbc_output *p = list;
	while (p!=NULL){
		printf("\nERR_DUPLICATE_CODE:\n");
		printf ("%s\n", p->err_msg);
		p = p->next;
		
	}
}


/*
 * run_tool (every module contains this function)
 *
 * Runs a tool over the sources/executables and parses its output
 * in order to extract all the errors reported by the tool.
 *
 * returns: the list of errors detected by the tool
 * param1: input = pointer to the information required by the 
 * tool (sources and/or executables)
 * param2: flags = a bit set that indicates what errors are tracked
 * at the moment of usage
 * param3: err_count = will hold the number of errors detected 
 */

struct rbc_output *
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count)
{
	char command[LINE_MAX] = DEFAULT_CMD;
	char line[LINE_MAX], message[MSG_SIZE], *p;
	struct rbc_static_input *static_input = NULL;
	struct rbc_output *output = NULL;
	struct rbc_output node;
	FILE *f;
	int i;

	if (!ISSET_ERR(ERR_DUPLICATE_CODE, flags))
		return NULL;

	node.err_msg = 0x01;
	*err_count = 0;

	if (input != NULL && input->input_ptr!=NULL && input->tool_type == STATIC_TOOL) {
		static_input = (struct rbc_static_input *) input->input_ptr;
		for (i = 0; i < input->args_count; i++) {
			strcat(command, SPACE);
			strcat(command, input->tool_args[i]);
		}
		for (i = 0; i < static_input->file_count; i++) {
			strcat(command, SPACE);
			strcat(command, static_input->file_names[i]);
		}
			
		strcat(command, OUTPUT);
		system(command);

		f = fopen(OUTPUT_FILE, "rt");
		if (f == NULL) {
			return NULL;
		}
		while (fgets(line, LINE_MAX, f) != NULL) {
			if (strstr(line, "Found") && node.err_msg == NULL) {
				node.err_msg = strdup(message);
				node.err_type = ERR_DUPLICATE_CODE;
				add(&output, node);
				node.err_msg = 0x01;
			}
			if (strstr(line, "duplicate lines in the following files:") &&
					node.err_msg == 0x01) {
				node.err_msg = NULL;
				memset(message, 0, MSG_SIZE);
				strcpy(message, "Duplicate lines:");
				continue;
			}
			if (strstr(line, "Between lines") && node.err_msg == NULL) {
				int from, to;
			        char name[MSG_SIZE];

				memset(name, 0, MSG_SIZE);
				sscanf(line, " Between lines %d and %d in %s\n", &from, &to, name);
				sprintf(message + strlen(message), " %s[%d-%d] ", name, from, to);
			}

		}
		fclose(f);
		//print_list(output);
		system(RM_OUTPUT);
	}

	return output;
}
