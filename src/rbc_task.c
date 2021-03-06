 
#include <stdio.h>
#include <stdlib.h>

#include "../lib/rbc_task.h"
#include "../include/utils.h"

#ifdef _WIN32
	#ifndef popen
		#define popen _popen
		#define pclose _pclose
	#endif
#endif

rbc_task_t *
open_process (int argc, char **args, enum EN_rbc_access access)
{
	rbc_task_t *task = NULL;
	char *comm = NULL;
	FILE *comm_output = NULL;
	
	comm = make_comm_string (argc, args);
	if (comm != NULL)
	{
		// start process
		// TODO: set R/W according to the given access parameter
		comm_output = popen(comm, "r");
		if (comm_output != NULL)
		{
			task = (rbc_task_t *) rbc_get_mem(1, sizeof *task);
			if (task != NULL)
			{
				task->task_output = comm_output;
				task->access = access;
			}
		}
		else
		{
			log_message (INVALID_PROC_STARTED, NULL);
		}
		
		rbc_free_mem ((void **)&comm);
	}
	
	return task;
}

int
wait_process (rbc_task_t *task)
{
	int status = 0;
	
	if (task != NULL)
	{
		if (task->task_output != NULL)
		{
			// close and wait for process finalization
			status = pclose (task->task_output);
			if (status != 0)
			{
				log_message (INVALID_PROC_STOPED, NULL);
			}
		}
		
		rbc_free_mem ((void **)&task);
	}
	else
	{
		status = -1;
	}
	
	return status;
}

