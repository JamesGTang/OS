#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/*
Default version of tshell using linux system() call

*/

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
			system(line);
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