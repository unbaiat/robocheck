/*
 * rbc_valgrind.c: Valgrind module
 *
 * General description: 
 * 	Checks for invalid memory access, memory leaks, usage of 
 * uninitialized variables and unclosed file descriptors.Parses the 
 * output of valgrind according to the bit set of tracked errors 
 * (user can choose to track a part, none or all types of errors).
 *
 * (C) 2011, Iulia Bolcu <reea_mod@yahoo.com>
 *              * last review 27.06.2011
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/dynamic_tool.h"

#define LINE_MAX 512
#define SEPARATORS " :()\r\n\t"
#define DEFAULT_CMD "valgrind --log-file=fil\%p --leak-check=full"
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
	char *s;
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
			case ERR_FILE_DESCRIPTORS:
				printf("\nOPENED, BUT NOT CLOSED FILE DESCRIPTOR:\n");
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
			strcpy(error_message, " In function ");
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
 * file_descriptors
 *
 * Parses the lines containing information  about file descriptors.
 * Due to it's dificulty had to be implemented separatly.
 *
 * returns: nothing
 * param1: g = pointer to file pointer (a file that contains the output of the tool)
 * param2: first_line = we must keep the reference to the first line read
 * in order to parse
 * param3: output = pointer to the list of errors. 
 */

static void 
file_descriptors(FILE **g,char *first_line,struct rbc_output **output){
	char line[LINE_MAX];
	char *f_name,*s_name,*l_number,*p;
	int nr_fds,i,j;
	struct rbc_output node;
	node.err_msg = NULL;
	p = strtok(first_line, SEPARATORS);
	if (p==NULL) 
		return;
	for (i=0;i<3;i++){
		p = strtok(NULL, SEPARATORS);
		if (p==NULL) 
			return;
	}

	nr_fds = atoi(p);

	for (i=0;i<nr_fds;i++){
		do{
			if (fgets(line, LINE_MAX, *g) == NULL){
				return;
			}
		}while(is_break_line(line));
		p = strtok(line, SEPARATORS);
		if (p == NULL) return;
		for (j=0;j<5;j++){
			p = strtok(NULL, SEPARATORS);
			if (p == NULL) 
				goto void_fd;
		}
		node.err_msg = strdup(p);
		if (fgets(line, LINE_MAX, *g) == NULL){
			return;
		}
		if (!strstr(line,"<inherited from parent>")){
			node.err_type= ERR_FILE_DESCRIPTORS;
			add(output,node);
		}
		else{
			free(node.err_msg);
		}

		void_fd:
		do{
			if (fgets(line, LINE_MAX, *g) == NULL){
				return;
			}
			
		}while (!is_break_line(line));
				
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
	char command[LINE_MAX]=DEFAULT_CMD;
	char line[LINE_MAX],name[LINE_MAX],*aux;	
	struct rbc_dynamic_input *dynamic_input = NULL;
	struct rbc_output *output = NULL;
	FILE *f,*g;
	int i;

	*err_count = 0;
	if (input != NULL && input->input_ptr!=NULL && input->tool_type == DYNAMIC_TOOL){
		
		dynamic_input = (struct rbc_dynamic_input *) input->input_ptr;
		for(i=0;i<input->args_count;i++){
			strcat(command,SPACE);
			strcat(command,input->tool_args[i]);
		}
			
		strcat(command,SPACE);
		strcat(command,dynamic_input->exec_name);
		strcat(command,SPACE);
		strcat(command,DEV_NULL);
		system(command);

		f=popen(LIST_OUTPUT_FILES,"r");
		if (f==NULL)
			return NULL;	
		while (fgets(name, LINE_MAX, f) != NULL){
			aux = strtok(name,"\n\t\r ");
			g = fopen(aux,"rt");
			if (g == NULL) {
				return NULL;
			}

			while (fgets(line, LINE_MAX, g) != NULL){
				
				if (ISSET_ERR(ERR_MEMORY_LEAK, flags) 
					&& strstr(line,"bytes in") 
					&& strstr(line,"blocks are definitely lost in loss record")){
					get_info(&g,dynamic_input,&output,ERR_MEMORY_LEAK);
					continue;
				}
					
				if (ISSET_ERR(ERR_INVALID_ACCESS, flags)
					&& (strstr(line,"Invalid write of size")
					|| strstr(line,"Invalid read of size"))){
					get_info(&g,dynamic_input,&output,ERR_INVALID_ACCESS);
					continue;	
				}
				if (ISSET_ERR(ERR_UNINITIALIZED, flags)
					&& strstr(line,"Use of uninitialised value")){
					get_info(&g,dynamic_input,&output,ERR_UNINITIALIZED);
					continue;
				}
					
				if (ISSET_ERR(ERR_FILE_DESCRIPTORS, flags)
					&& strstr(line,"FILE DESCRIPTORS:")){
					file_descriptors(&g,line, &output);
					continue;				
				}
				
			}
			fclose(g);

		}
		pclose(f);
		print_list(output);
		system(REMOVE_OUTPUT_FILES);
		
	}

	
	return output;
}
