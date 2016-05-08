#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
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


// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6

bool yieldTest() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	pid = fork();
	if(pid != 0) {
		printf("father pid: %d\n", getpid());
		// Parent: makes child a SHORT
		sp.sched_priority = 0;
        sp.requested_time = 1000;
        sp.num_cooloff = 1; 
		sched_setscheduler(pid, SCHED_SHORT, (struct sched_param *)&sp); //makes son a short
		while(sched_getscheduler(getpid()) != SCHED_SHORT) {}
		printf("4\n"); //father is better than son, he should run now
		sched_yield();
		printf("5\n"); //although father yielded he should still run before son since father is a better SHORT
		usleep(100000); //father sleeps for 100ms
	}
	else {
		printf("son pid: %d\n", getpid());
        // Child, a SHORT.
        while(sched_getscheduler(getpid()) != SCHED_SHORT) {}
		printf("1\n"); //son is SHORT and should run first, before OTHER father
		sched_yield();
		printf("2\n"); //although son yielded he should still run before father since father is OTHER
		sp.sched_priority = 0;
        sp.requested_time = 1000;
        sp.num_cooloff = 1; 
		sched_setscheduler(getppid(), SCHED_SHORT, (struct sched_param *)&sp); //makes father an other
		printf("3\n"); // father isn't better than son, we should not switch to father here
		nice(1); //makes son a bit worse than father
		printf("6\n"); //father is in sleep, son should run now
		exit(0);
    }
	waitpid(pid, &status, 0);
	return true;
		
}

int main() {
	printf("yieldTest output:\n");
	yieldTest();
	return 0;
}	
