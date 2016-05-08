#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "hw2_syscalls.h"
#include "test_utilities.h"

#define SCHED_SHORT 5

struct sched_param_ex {
    int sched_priority;
    int requested_time;
    int num_cooloff;
};

void busyWait(int ms)
{/*
    struct timespec tstart, tend;
    long int runtime;

    clock_gettime(CLOCK_REALTIME, &tstart);
    for(;;) {
        clock_gettime(CLOCK_REALTIME, &tend);
        runtime = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
        if(runtime >= ms) {
            return;
        }
    }*/
    int i = 200*ms;
    while(--i > 0);
}
#define CLOCKS_PER_SEC	100	/* frequency at which times() counts */
#define MS_BETWEEN_CLOCKS(c1,c2) (((double) ((c2) - (c1))) / (CLOCKS_PER_SEC/1000))
static void busywait(int delay_milliseconds){
	clock_t start,end;
	start = clock();
	end = clock();
	while (MS_BETWEEN_CLOCKS(start,end) < delay_milliseconds)
	{
		end = clock();
	}
}

// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6 7

bool RtVsShortAndOverdue() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	pid = fork();
	if(pid != 0) {
		// Parent: makes child a SHORT and father a FIFO (RT priority)
		printf("son:%d ",pid);
		printf("parent:%d\n",getpid());
		is_SHORT(-666);
		sp.sched_priority = 20;
        sp.requested_time = 0;
        sp.num_cooloff = 0;
		sched_setscheduler(getpid(), SCHED_FIFO, (struct sched_param *)&sp); 
		sp.requested_time = 100;
        sp.num_cooloff = 3;
		sched_setscheduler(pid, SCHED_SHORT, (struct sched_param *)&sp);
		printf("1\n"); //father is RT and should run before SHORT son
		busywait(100); 
		printf("2\n"); //father should still run
		usleep(10000); //father sleeps for 10ms so son should take his place
		printf("4\n");
		usleep(120000); //father sleeps for 120ms, son should take his place 
		printf("6\n");
		usleep(50000); //father sleeps for 50ms so son should take his place
		is_SHORT(-777);
	}
	else {
        // Child, a SHORT.
        while(sched_getscheduler(getpid()) != SCHED_SHORT) {}
		printf("3\n"); 
		busywait(20); // son is still regular SHORT but father should wake up by now and take his place 
		printf("5\n");
		busywait(140); // son becomes overdue by now and father should wake up and take his place
		printf("7\n");
		exit(0);
    }
	waitpid(pid, &status, 0);
	return true;
		
}


int main() {
	printf("RtVsShortAndOverdue output:\n");
	RtVsShortAndOverdue();
	return 0;
}	
