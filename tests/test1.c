#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main()
{
	int fd[10];
	char *p = NULL;

	switch(fork())
	{
		case 0:
			printf ("In child\n");
			fd[0] = open("test.c", O_RDWR);
			break;
		case -1:
			perror("fork");
			exit(EXIT_FAILURE);
		default:
			printf ("In parrent\n");
			p = malloc(20);
			p[20] = 'a';
	}

	return 0;
}
