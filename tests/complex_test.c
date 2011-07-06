#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>

#define ZECE 10

int contor = 0;
pthread_mutex_t mutex;	
void * thread_function(void * args) {
    contor ++;
    return NULL;
}



int main(){
	int i,rc;
	int buf[10];
	char *y;
	char ana[30];
	unsigned int a = 0xbabe;
	pthread_t a_thread[10];
	pthread_mutex_t mutex_main;
	FILE * f;
	
	f = fopen("test.in","rt");
	pthread_mutex_unlock(&mutex_main);
	for (i=0;i<10;i++){
		rc = pthread_create(&a_thread[i], NULL, thread_function, NULL);
	}	
	for(i=0;i<10;i++){
		if (pthread_join(a_thread[i], NULL))
			perror("pthread_join");
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);	
	pthread_mutex_destroy(&mutex);


	
	y = malloc(10);	
	buf[ZECE+1] = 3;
	printf("%s %x\n","robocheck test",a);
	
	rc = a;

	y=malloc(4);
	return 0;
} 
