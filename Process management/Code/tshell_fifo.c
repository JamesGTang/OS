#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
/*
tshell using fork() and FIFO
*/

// function macros
int systemf(char *cmd);

// defintions
#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"


int main(){
	struct timespec start, stop;
	clock_gettime(CLOCK_REALTIME,&start);
	while(1){
		char *line;
		double acc;
		size_t len = 100;
		size_t read;
		line = (char *)malloc(len*sizeof(char));
		printf("\nTyped a command: ");
		if ((read = getline(&line,&len,stdin)) !=-1){
			printf("%s",line);
			systemf(line);
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


int systemf(char *cmd){
	pid_t pid= fork();
	int status;
	if(pid==0){
		const char *argv[4];
		argv[0] = SHELL_NAME;
		argv[1] = "-c";
		argv[2] = strcat(cmd,">fifo");
		argv[4] = NULL;
		// child process
		execve(SHELL_PATH,(char *const*)argv,__environ);
	}
	else if(pid<0)
		status =-1;
	else{
		// parent wait for execution of child
		if (waitpid(pid,&status,0) != pid){
			status = -1;
		}
	}
	return status;
}