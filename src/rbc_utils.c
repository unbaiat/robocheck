
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "../lib/rbc_constants.h"
#include "../lib/rbc_utils.h"

void
create_log_message (char * message)
{
	int status = 0;
	int max_length = 0, current_length = 0;
	struct timeval time_val; struct tm *time_struct = NULL;
	time_t *crt_time = NULL;

	// adjust message pointer and maximum length
	message = (message != NULL) ? message : NULL_STRING;
	max_length = MAX(2 * MAX_BUFF_SIZE, strlen(message) + 1);
	
	// reset the buffer
	memset (LoggerBuff, 2 * MAX_BUFF_SIZE, 0);
	
	status = gettimeofday (&time_val, NULL);
	if (status == 0)
	{
		crt_time = &(time_val.tv_sec);
	}
	
	// set time stamp
	if (crt_time != NULL)
	{
		time_struct = localtime(crt_time);
		if (time_struct != NULL)
		{
			current_length = strftime (LoggerBuff, max_length, "%d-%m-%Y %T - ", time_struct);
			if (current_length == 0)
			{
				memset (LoggerBuff, 2 * MAX_BUFF_SIZE, 0);
			}
		}
	}
	max_length -= current_length;
	
	// append the robocheck tag
	strncat (LoggerBuff, CurrentModule, max_length);
	current_length = strlen (LoggerBuff);
	max_length -= current_length;
	
	// append the message contents
	snprintf (LoggerBuff + current_length, max_length - 1, " %s\n", message);
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

