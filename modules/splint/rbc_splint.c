/*
 * rbc_splint.c: splint module
 *
 * General description: 
 * 	Checks for memory leaks, invalid memory acces for both static 
 * and dynamic buffers, uninitialized variables, global variables used only
 * inside module but not declared static and signed/unsigned variables 
 * assignment.
 *	Parses the output of splint according to the bit set, flags.
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
#define SEPARATORS " :()\r\n\t"
#define ASSIGN_SEPARATORS " ,:\r\n"
#ifdef _WIN32
	#define DEFAULT_CMD "c:\\splint-3.1.1\\bin\\splint.exe "
#else
	#define DEFAULT_CMD "splint"
#endif
#define OUTPUT " > output"
#define SPACE " "
#define OUTPUT_FILE "output"

#ifdef _WIN32
	#define RM_OUTPUT "del output"
#else
	#define RM_OUTPUT "rm output"
#endif


extern FILE *FileLogger;
static char *function=NULL;

/*
 * get_function
 *
 * Extracts the name of the function.
 *
 * returns: (nothing)
 * param1: line = the parsed line (containing the function name)
 */

static void 
get_function(char *line){
	int i=0;
	char *p = strtok(line, SEPARATORS);
	if (p == NULL) 
		return;
	for(i=0;i<3;i++){
		p = strtok(NULL, SEPARATORS);
		if (p == NULL) 
			return;
	}
	if (function != NULL)
		free(function);
	function = strdup(p);
}

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
		switch (p->err_type){
			case ERR_MEMORY_LEAK:
				printf ("\nMEMORY LEAK:\n");
				break;
			case ERR_INVALID_ACCESS:
				printf ("\nINVALID ACCESS:\n");
				break;
			case ERR_UNINITIALIZED:
				printf("\nUNINITIALIZED VARIABLE USAGE:\n");
				break;
			case ERR_STATIC_VARIABLE:
				printf("\nSTATIC VARIABLE\n");
				break;
			case ERR_SIGNED_UNSIGNED:
				printf("\nERR_SIGNED_UNSIGNED\n");
				break;
			default:
				break;
		}
		printf("%s\n",p->err_msg);		
		p = p->next;
		
	}
}

/*
 * get_info
 *
 * Parses a line and extracts information about the 
 * error (source, function, line, etc)
 *
 * returns: nothing
 * param1: line = output line that will be parsed
 * param2: case_static = need to distinguish beetween
 * the case of a global variable (declared in the module, used
 * only inside of it but not declared static) and other parsing cases.
 * param3: output = pointer to the list of errors. 
 * param4: err_type = specifies the type of error analyzed
 */

static void 
get_info(char *line,int case_static,struct rbc_output **output,enum EN_err_type err_type){
	char error_message[LINE_MAX];
	char *s_name,*l_number;
	struct rbc_output node;
	s_name = strtok(line, SEPARATORS);
	if (s_name == NULL) 
		return;
	l_number = strtok(NULL, SEPARATORS);
	if (l_number == NULL) 
		return;
	if (case_static || function == NULL){
		strcpy(error_message, "In file ");
	}
	else{
		strcpy(error_message, "In function ");
		strcat(error_message, function);
		strcat(error_message, ", in file ");
	}	
	strcat(error_message, s_name);
	strcat(error_message, ", at line ");
	strcat(error_message, l_number);
	node.err_type = err_type;
	node.err_msg = strdup(error_message);
	add(output,node);
}

/*
 * is_signed_unsigned
 *
 * Determines whether an assignment occurs beetween
 * the signed and unsigned of a data type.
 *
 * returns: 1- true/0 -false
 * param1: line = output line that will be parsed
 */

static int
is_signed_unsigned(char *line){
	char *p,*left,*right;
	int count_unsigned =0;
	int i; 
	int limit = 0;
	if (strstr(line,"Assignment of")){
		limit = 5;
        }
	else limit = 8;
		
	p = strtok(line, ASSIGN_SEPARATORS);
	if (p == NULL)
		return 0;
	for (i=0;i<limit;i++){
		p = strtok(NULL, ASSIGN_SEPARATORS);
		if (p == NULL)
			return 0;
	}
	if (strcmp(p,"unsigned") == 0){
		count_unsigned ++;
		p = strtok(NULL, ASSIGN_SEPARATORS);
		if (p == NULL)
			return 0;
	}
	left = p;
	for (i=0;i<2;i++){
		p = strtok(NULL, ASSIGN_SEPARATORS);
		if (p == NULL)
			return 0;
	}

	if (strcmp(p,"unsigned") == 0){
		count_unsigned ++;
		p = strtok(NULL, ASSIGN_SEPARATORS);
		if (p == NULL)
			return 0;
	}
	right = p;
	return (strcmp(right,left) == 0 
		&& count_unsigned == 1);

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
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count){
	char line[LINE_MAX],scd_line[LINE_MAX];
	char assignments[2*LINE_MAX];	
	char command[LINE_MAX]=DEFAULT_CMD;
	struct rbc_static_input *static_input = NULL;
	struct rbc_output *output = NULL;
	struct rbc_output node;
	FILE *f;
	int i;
	*err_count = 0;
	node.err_msg = NULL;
	if (input != NULL  && input->input_ptr != NULL && input->tool_type == STATIC_TOOL){
		static_input = (struct rbc_static_input *) input->input_ptr;
		for(i=0;i<input->args_count;i++){
			strcat(command,SPACE);
			strcat(command,input->tool_args[i]);
		}

		for(i=0;i<static_input->file_count;i++){
			strcat(command,SPACE);
			strcat(command,static_input->file_names[i]);
		}
			
		strcat(command,OUTPUT);
		system(command);

		f=fopen(OUTPUT_FILE,"rt");
		if (f==NULL){
			return NULL;
		}	

		while (fgets(line, LINE_MAX, f) != NULL){
			if (strstr(line,"(in function")){
				get_function(line);
				continue;
			}
			if (ISSET_ERR(ERR_STATIC_VARIABLE, flags)
				&& (strstr(line,"Variable exported but not used")
				|| strstr(line,"Function exported but not used"))	
				){
				get_info(line,1,&output,ERR_STATIC_VARIABLE);
				continue;
			}
			if (ISSET_ERR(ERR_MEMORY_LEAK, flags)
				&& strstr(line,"Fresh storage")
				&& strstr(line,"created")){
				get_info(line,0,&output,ERR_MEMORY_LEAK);
				continue;
			}
			if (ISSET_ERR(ERR_UNINITIALIZED, flags)
				&& strstr(line,"used before definition")){
				get_info(line,0,&output,ERR_UNINITIALIZED);
				continue;
			}
			if (ISSET_ERR(ERR_INVALID_ACCESS, flags)
				&& (strstr(line,"Likely out-of-bounds")
				|| strstr(line,"Possible out-of-bounds"))
				){
				get_info(line,0,&output,ERR_INVALID_ACCESS);
				continue;
			}
			if (ISSET_ERR(ERR_SIGNED_UNSIGNED, flags)
				&& ( strstr(line,"Assignment of")
				|| ( strstr(line,"initialized to type") && strstr(line,"expects"))
				)){
				strcpy(assignments,line);
				if (fgets(scd_line, LINE_MAX, f) != NULL)
					strcat(assignments,scd_line);
				if (is_signed_unsigned (assignments))
					get_info(line,0,&output,ERR_SIGNED_UNSIGNED);
				continue;
			}
			
		
		}

		fclose(f);
		system(RM_OUTPUT);
	}

	return output;
}
