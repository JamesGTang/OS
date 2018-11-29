#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include<sched.h>
/*
tshell using clone()
*/

// function macros
int systemvc(char *cmd);
int child_func(void *arg);
// defintions
#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define STACK_SIZE (1024 * 1024)

int main(){
	struct timespec start, stop;
	clock_gettime(CLOCK_REALTIME,&start);
	while(1){
		char *line;
		size_t len = 100;
		size_t read;
		line = (char *)malloc(len*sizeof(char));
		printf("\nTyped a command: ");
		if ((read = getline(&line,&len,stdin)) !=-1){
			printf("%s",line);
			systemvc(line);
		}
		else{
			printf("%s","\nNo more command, exiting\n");
			clock_gettime(CLOCK_REALTIME,&stop);
			double acc = (((stop.tv_sec * 1000000000 + stop.tv_nsec) - (start.tv_sec * 1000000000+start.tv_nsec)));
			printf("%lf\n",acc);
			exit(0);
		}	
	}
}


int systemvc(char *cmd){
	/* Allocate stack for child */
	char *stack = malloc(STACK_SIZE);
	char *stackTop;
	int status = 0;
    if (stack == NULL)
        printf("Error malloc\n");
     stackTop = stack + STACK_SIZE;
     int pid = clone(child_func,stackTop, CLONE_FS,cmd);
     if(pid<0){
     	printf("Unable to create child process\n");
     }
     if (waitpid(pid,&status,0) != pid){
		status = -1;
	}
	return status;
}

int child_func(void *arg){
	const char *argv[4];
	argv[0] = SHELL_NAME;
	argv[1] = "-c";
	argv[2] = arg;
	argv[4] = NULL;
	execve(SHELL_PATH,(char *const*)argv,__environ);
}