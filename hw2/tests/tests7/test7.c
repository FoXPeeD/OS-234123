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

void busywait(int ms)
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

// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6 7 8 9 10 11

bool otherVsShortAndOverdue() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	pid = fork();
	if(pid != 0) {
		// Parent: makes child a SHORT
		sp.sched_priority = 0;
        sp.requested_time = 100;
        sp.num_cooloff = 1; 
		sched_setscheduler(pid, SCHED_SHORT, (struct sched_param *)&sp);
		printf("2\n");
		busywait(20); // father runs for at least 10ms, should be enough time for son to wake up and take his place
		printf("4\n");
		busywait(300); // this is probably large enough for father to change to expired but he still needs to run since son is overdue
		printf("5\n");
		usleep(120000); //father sleeps for more than 100ms so son should become SHORT again by the time father wakes up
		printf("8\n"); //son has become overdue again, we switched to father
		usleep(110000); //father sleeps for 110ms
		printf("10\n"); //son is overdue because he has no cooloffs so father should run
	}
	else {
        // Child, a SHORT.
        while(sched_getscheduler(getpid()) != SCHED_SHORT) {}
		printf("1\n"); //son is SHORT and should run first, before OTHER father
		usleep(10000); //puts child to sleep for 10ms, should be enough time to context_switch to father
		printf("3\n");
		busywait(110); // son runs for at least 100ms, should be enough time for son to become overdue and father needs to take his place
		printf("6\n");
		busywait(120); 
		printf("7\n"); //son is now regular SHORT again , we should not context switch back to father
		busywait(120); //son becomes overdue here
		printf("9\n"); //father sleeps so son should run next
		busywait(110); //son doesn't become SHORT during this since we had only 1 cooloff, and father wakes up here
		printf("11\n");
		exit(0);
    }
	waitpid(pid, &status, 0);
	return true;
		
}


int main() {
	printf("otherVsShortAndOverdue output:\n");
	otherVsShortAndOverdue();
	return 0;
}
	
