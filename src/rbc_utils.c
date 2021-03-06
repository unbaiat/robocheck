#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/utils.h"
#include "../lib/rbc_constants.h"
#include "../lib/rbc_utils.h"

#ifdef _WIN32
	#ifndef popen
		#define popen _popen
		#define pclose _pclose
	#endif
#endif

#define LINE_MAX	512

void
create_log_message (char * message)
{
	int max_length = 0, current_length = 0, len = 0, i;
	time_t tt;
	struct tm *time_struct = NULL;

	// adjust message pointer and maximum length
	message = (message != NULL) ? message : NULL_STRING;
	max_length = MAX(2 * MAX_BUFF_SIZE, strlen(message) + 1);
	
	// reset the buffer
	memset (LoggerBuff, 0, 2 * MAX_BUFF_SIZE);
	
	tt = time(&tt);
	time_struct = localtime(&tt);
	if (time_struct != NULL) {
	#ifdef _WIN32
		current_length = sprintf(LoggerBuff, "%02d-%02d-%d %02d:%02d:%02d - ",
			time_struct->tm_mday, time_struct->tm_mon, time_struct->tm_year + 1900,
			time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
	#else
		current_length = strftime(LoggerBuff, max_length, "%d-%m-%Y %T - ", time_struct);
	#endif
		if (current_length == 0) {
			memset (LoggerBuff, 0, 2 * MAX_BUFF_SIZE);
		}
	}
	max_length -= current_length;
	
	// append the robocheck tag
	strncat (LoggerBuff, CurrentModule, max_length);
	current_length = strlen (LoggerBuff);
	max_length -= current_length;
	
	// append the message contents
	len = strlen(message);
	if (len > max_length - 1)
		len = max_length - 1;
	sprintf(LoggerBuff + current_length, " ");
	for (i = 0; i < len; i++)
		sprintf(LoggerBuff + current_length + 1 + i, "%c", message[i]);
}

int
is_internal_stream (FILE *stream)
{
	return  (stream == STDERR_HANDLER) ||
		(stream == STDIN_HANDLER)  ||
		(stream == STDOUT_HANDLER);
}

void
set_robocheck_module(void)
{
	sprintf(CurrentModule, "%s", RBC_TAG);
}

void
set_running_module(char *libname)
{
	char *dup_libname = NULL;
	char *tool_name = NULL, *end_pos = NULL;
	int end_pos_index = -1;
	
	if (libname == NULL)
	{
		return;
	}

	dup_libname = strdup(libname);
	if (dup_libname == NULL)
	{
		return;
	}

	tool_name = strstr(dup_libname, LIB_START);
	tool_name = (tool_name != NULL) ? (tool_name + strlen(LIB_START)) : dup_libname;

	end_pos = strstr(tool_name, LIB_END);
	if (end_pos != NULL)
	{
		end_pos_index = end_pos - tool_name;
		*(tool_name + end_pos_index) = '\0';
	}

	sprintf(CurrentModule, "[%s]", tool_name);

	free(dup_libname);
}

