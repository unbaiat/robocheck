/*
 * rbc_helgrind.c: helgrind module
 *
 * General description: 
 * 	Checks for several sycronization problems like deadlocks,
 * data races, destruction of invalid/locked mutexes, misusage of 
 * conditional variables, exiting threads that hold locks etc. 
 * Parses the output of valgrind according to the bit set of 
 * tracked errors (user can choose to track a part, 
 * none or all types of errors).
 *
 * (C) 2011, Iulia Bolcu <reea_mod@yahoo.com>
 *              * last review 10.07.2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/dynamic_tool.h"
#define LINE_MAX 512
#define SEPARATORS " :()\r\n\t"
#define SEPARATORS_SHARP " #:()\r\n\t"
#define DEFAULT_CMD "valgrind --log-file=fil\%p --tool=helgrind "
#define SPACE " "
#define DEV_NULL " > /dev/null"
#define LIST_OUTPUT_FILES "ls fil*"
#define REMOVE_OUTPUT_FILES "rm fil*"

/*
 * is_source
 *
 * Searches for a source name in a list of sources.
 *
 * returns: 1 - found /0 - not found
 * param1: sources = the list of source names
 * param2: source_count = the number of sources in the list
 * param3: source = the string that is searched in the list
 */

static int
is_source (const char **sources, int source_count, char * source){
	int i;
	const char *s;
	if (sources != NULL) {
		for (i = 0;i < source_count;i++){
			s = strrchr(sources[i],'/');
			if (s != NULL) 
				s++;
			else 
				s = sources[i];
			if (strcmp(source,s)==0){
				return 1;
			}
		}
	}
        
	return 0;
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
print_list(struct rbc_output *lista){
	struct rbc_output *p = lista;
	while (p!=NULL){
		switch (p->err_type){
			case ERR_DATA_RACE:
				printf ("\nERR_DATA_RACE:\n");
				printf ("%s\n",p->err_msg);
				break;
			case ERR_DEAD_LOCK:
				printf ("\nERR_DEAD_LOCK:\n");
				break;
			case ERR_UNLOCK:
				printf("\nERR_UNLOCK:\n");
				break;
			case ERR_DESTROY:
				printf("\nERR_DESTROY:\n");
				break;
			case ERR_HOLD_LOCK:
				printf("\nERR_HOLD_LOCK:\n");
				break;
			case ERR_CONDITION_VARIABLE:
				printf("\nERR_CONDITION_VARIABLE:\n");
				break;
			default:
				break;
		}

		printf("%s\n",p->err_msg);
		p = p->next;
		
	}
}

/*
 * is_break_line
 *
 * Determines if a line in output is "empty".
 * "Empty" means that the line contains only the pid
 * of the process(every line starts with this information).
 *
 * returns: 1 - true /0 - false
 * param1: line = the line read from output file
 */

static int
is_break_line(char *line){
	char copy_buffer[LINE_MAX],*p;
	strcpy(copy_buffer,line);
	p = strtok(copy_buffer, SEPARATORS);
	if (p == NULL)
		return 1;
	p = strtok(NULL, SEPARATORS);
	if (p == NULL) 
		return 1;
	else return 0;
}


/*
 * is_detail_line
 *
 * Determines if a line in output is "detailed".
 * "Detailed" means that the line  has the format 
 *  <pid> "at" <function_name> (<source_name>:<line_number>)
 *	or
 *  <pid> "by" <function_name> (<source_name>:<line_number>)
 * Basically testes whether the second token of the output line is 
 * one of the keywords "at" or "by".
 *
 * returns: 1 - true /0 - false
 * param1: line = the line read from output file
 */

static int
is_detail_line(char *line){
	char copy_buffer[LINE_MAX],*p;
	strcpy(copy_buffer,line);
	p = strtok(copy_buffer, SEPARATORS);
	if (p == NULL) return 0;
	p = strtok(NULL, SEPARATORS);
	if (p == NULL) return 0;
	return (strcmp(p,"at")==0 || strcmp(p,"by")==0);
}

/*
 * parse_line
 *
 * Extracts the three basic informations about an error:
 * - the function it apears in
 * - the source name
 * - the line number
 *
 * returns: 1 - parse was successful /0 - otherwise
 * param1: line = the line read from output file
 * param2: f_name = will be filled with function name
 * param3: s_name = will be filled with source name
 * param4: l_number = will be filled with line number
 *
 */

static int
parse_line(char *line,char **f_name,char **s_name,char**l_number){
	char *p = strtok(line, SEPARATORS);
	int i;
	if (p == NULL)
		return 0;
	for (i=0;i<3;i++){
		p = strtok(NULL, SEPARATORS);
		if (p == NULL) 
			return 0;
	}
	*f_name = p;
	*s_name = strtok(NULL, SEPARATORS);
	if ((*s_name) == NULL)
		return 0;
	*l_number = strtok(NULL, SEPARATORS);
	if ((*l_number) == NULL)
		return 0;
	return 1;
}


/*
 * data_race
 *
 * Extracts the details about a data race.
 *
 * returns: nothing
 * param1: g = pointer to the file pointer
 * param2: dynamic_input = useful to determine of the source mentioned in the stack
 * trace of the error is part of the verified homework or a part of standard libraries. 
 * param3: output = pointer to the list of errors. 
 *
 */

static void 
data_race(FILE **g,struct rbc_dynamic_input *dynamic_input,struct rbc_output **output){
	char line[LINE_MAX];
	char *f_name,*s_name,*l_number,*p;
	struct rbc_output node;
	char  error_message[LINE_MAX];
	int found_a_file = 0;
	node.err_msg = NULL;
	while (fgets(line, LINE_MAX, *g) != NULL
		&& !is_break_line(line)){
		if (strstr(line,"This conflicts with a previous")){
			goto conflicts_with;	
		}
		if (!is_detail_line(line))
			return;
		if (!parse_line(line,&f_name,&s_name,&l_number))
			return;
		if (!found_a_file &&
			is_source(dynamic_input->sources, dynamic_input->source_count,s_name)){
			found_a_file = 1;			
			strcpy(error_message, "In function ");
			strcat(error_message, f_name);
			strcat(error_message, ", in file ");
			strcat(error_message, s_name);
			strcat(error_message, ", at line ");
			strcat(error_message, l_number);
			node.err_msg = strdup(error_message);
			node.err_type = ERR_DATA_RACE;	
		}
			
	}

conflicts_with:
	while (fgets(line, LINE_MAX, *g) != NULL
		&& !is_break_line(line)){
		if (!is_detail_line(line))
			return;
		if (!parse_line(line,&f_name,&s_name,&l_number))
			return;
		if (is_source(dynamic_input->sources, dynamic_input->source_count,s_name)){
			strcat(error_message, "\nConflicts with data located in function ");
			strcpy(error_message,f_name);
			strcat(error_message,", in file ");
			strcat(error_message,s_name);
			strcat(error_message,", at line ");
			strcat(error_message,l_number);
			node.err_msg = strdup(error_message);
			add(output,node);
			return;	
		}
		
			
	}
}

/*
 * get_info
 *
 * Parses a line and extracts information about the 
 * error (source, function, line, etc)
 *
 * returns: nothing
 * param1: g = pointer to file pointer (a file that contains the output of the tool)
 * param2: dynamic_input = useful to determine of the source mentioned in the stack
 * trace of the error is part of the verified homework or a part of standard libraries. 
 * param3: output = pointer to the list of errors. 
 * param4: err_type = specifies the type of error analyzed
 */

static void 
get_info(FILE **g,struct rbc_dynamic_input *dynamic_input,struct rbc_output **output,enum EN_err_type err_type){
	char line[LINE_MAX];
	char error_message[LINE_MAX];
	char *f_name,*s_name,*l_number;
	struct rbc_output node;
	int found_a_file = 0;
	node.err_msg = NULL;
	while (fgets(line, LINE_MAX, *g) != NULL
		&& !is_break_line(line)){
		if (!is_detail_line(line))
			return;
		if (!parse_line(line,&f_name,&s_name,&l_number))
			return;
		if (!found_a_file &&
			is_source(dynamic_input->sources, dynamic_input->source_count,s_name)){
			found_a_file = 1;			
			strcpy(error_message, "In function ");
			strcat(error_message, f_name);
			strcat(error_message, ", in file ");
			strcat(error_message, s_name);
			strcat(error_message, ", at line ");
			strcat(error_message, l_number);
			node.err_type = err_type;
			node.err_msg = strdup(error_message);
			add(output,node);
			return;	
		}
			
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
run_tool (struct rbc_input *input, rbc_errset_t flags, int *err_count){
	struct rbc_dynamic_input *dynamic_input = NULL;
	struct rbc_output *output = NULL;
	char command[LINE_MAX]=DEFAULT_CMD;
	int i;
	char line[LINE_MAX],name[LINE_MAX],*p;
	FILE *f,*g;
	*err_count = 0;
	struct rbc_output node;

	//return NULL;
	if (input != NULL && input->input_ptr!=NULL && input->tool_type == DYNAMIC_TOOL)
	{
		dynamic_input = (struct rbc_dynamic_input *) input->input_ptr;
					
		strcat(command,dynamic_input->exec_name);
		strcat(command,DEV_NULL);
		system(command);

		f=popen(LIST_OUTPUT_FILES,"r");
		if (f==NULL) return NULL;	
		
		while (fgets(name, LINE_MAX, f) != NULL){
			p = strtok(name,"\n\t\r ");
			g = fopen(p,"rt");
			if (g == NULL) {
				return NULL;
			}

			while (fgets(line, LINE_MAX, g) != NULL){
				if (ISSET_ERR(ERR_UNLOCK, flags) 
					&& (strstr(line,"unlocked a not-locked lock") 
					|| strstr(line,"unlocked an invalid lock")
					|| (strstr(line,"unlocked") && strstr(line,"currently held by thread")))){
					get_info(&g,dynamic_input,&output,ERR_UNLOCK);
					continue;
				}
				if (ISSET_ERR(ERR_DESTROY, flags) 
					&& (strstr(line,"pthread_mutex_destroy of a locked mutex") 
					|| strstr(line,"pthread_mutex_destroy with invalid argument")
					)){
					get_info(&g,dynamic_input,&output,ERR_DESTROY);
					continue;
				}

				if (ISSET_ERR(ERR_DEAD_LOCK, flags) 
					&& strstr(line,"lock order") 
					&& strstr(line,"violated")
					){
					get_info(&g,dynamic_input,&output,ERR_DEAD_LOCK);
					continue;
				}
					
				if (ISSET_ERR(ERR_CONDITION_VARIABLE, flags)
					&& strstr(line,"pthread_cond")
					&& (strstr(line,"called with mutex held by a different thread")
					 || strstr(line,"called with un-held mutex") 
					 || strstr(line,"called with invalid mutex"))		
 					){
					get_info(&g,dynamic_input,&output,ERR_CONDITION_VARIABLE);
					continue;
				}

				if (ISSET_ERR(ERR_DATA_RACE, flags)
					&& strstr(line,"Possible data race during")
					){
					data_race(&g,dynamic_input,&output);
					continue;
				}

				if (ISSET_ERR(ERR_HOLD_LOCK, flags)
					&& strstr(line,"Exiting thread still holds")
					){
					p = strtok(line, SEPARATORS_SHARP);
					if (p==NULL) 
						break;
					for (i=0;i<2;i++){
						p =strtok(NULL, SEPARATORS_SHARP);
						if (p==NULL) 
							break;
					}
					node.err_type = ERR_HOLD_LOCK;
					node.err_msg = strdup(p);
					add(&output,node);	
				}
						
			}
			fclose(g);
		}
		pclose(f);
		//print_list(output);
		system(REMOVE_OUTPUT_FILES);

	}
	return output;
}
