
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "../lib/rbc_constants.h"
#include "../lib/rbc_utils.h"

#define LINE_MAX	512


int
cmp_msg_file (char *m1, char *m2)
{
	char *name1 = NULL, *name2 = NULL;
	char *n1 = strdup(m1);
	char *n2 = strdup(m2);
	char *p1 = strcasestr(n1, "in file");
	char *p2 = strcasestr(n2, "in file");
	int ret = 0, len1 = 0, len2 = 0, len_start = 0;

	if (p1 == NULL || p2 == NULL)
		return 0;

	p1 += 8;
	p2 += 8;

	trim_whitespace(p1);
	trim_whitespace(p2);

	name1 = strtok(p1, ", ");
	name2 = strtok(p2, ", ");

	len1 = strlen(name1);
	len2 = strlen(name2);
	len_start = name1 - n1;

	if (strcasecmp(name1, name2) == 0)
		ret = 1;
	if (strstr(name1, name2))
		ret = 1;
	if (strstr(name2, name1))
		ret = 1;

	/* Compare beginning. */
	if (strncasecmp(n1, n2, len_start))
		ret = 0;

	free(n1);
	free(n2);

	/* Compare end. */
	n1 = strdup(m1);
	n2 = strdup(m2);
	if (strcasecmp(n1 + len_start + len1, n2 + len_start + len2))
		ret = 0;

	free(n1);
	free(n2);

	return ret;
}


void
trim_whitespace (char *str)
{
	char *whitespace = " \t\n\r";
	int len = strlen(str), i = len - 1;
	
	/* Trailing whitespace */
	while(i >= 0)
	{
		if (strchr(whitespace, str[i]) == NULL)
			break;
		str[i] = 0;
		i--;
		len--;
	}

	/* Starting whitespace */
	i = 0;
	while(i < len)
	{
		if (strchr(whitespace, str[i]) == NULL)
			break;
		i++;
	}

	if (i)
		memmove(str, str + i, len - i + 1);
}

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

