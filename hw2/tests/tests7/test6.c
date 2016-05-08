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


// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6 7

bool niceTest() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	sp.sched_priority = 0;
	sp.requested_time = 1000;
	sp.num_cooloff = 3;
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); //father becomes a SHORT
	pid = fork();
	if(pid != 0) {
		printf("2\n"); //son is worse so father should run now
		nice(4); //father makes himself worse than son
		printf("4\n"); //son is asleep so faher should run
		nice(-10); //father becomes much better than son now
		busywait(200); //son should wake up here
		printf("5\n"); //son woke up but father is better so he should keep running
		nice(2);  //make father a bit worse
		printf("6\n"); // father is still better than son so he should keep running
		usleep(100000); //father sleeps for 100ms
	

	}
	else {
        // Child, a SHORT.
        while(sched_getscheduler(getpid()) != SCHED_SHORT) {}
		printf("1\n"); //after SHORT forks child should run first
		nice(2); //son makes himself worse than father
		printf("3\n"); //father is worse so son should run now
		usleep(100000); //son sleeps for 100ms
		printf("7\n"); //father is asleep so son should run
		exit(0);
    }
	waitpid(pid, &status, 0);
	return true;
		
}

int main() {
	printf("niceTest output:\n");
	niceTest();
	return 0;
}	
