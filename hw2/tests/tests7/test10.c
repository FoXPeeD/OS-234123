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
    int i = 200*ns;
    while(--i > 0);
}

bool shortFork() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	sp.sched_priority = 0;
	sp.requested_time = 3000;
	sp.num_cooloff = 3;
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); //father becomes a SHORT
	pid = fork();  
	if(pid != 0) {
		ASSERT_TEST(remaining_time(getpid()) <= 1500); //if you didnt change sched_exit you could fail here!
		ASSERT_TEST(remaining_cooloffs(getpid()) == 1);

	}
	else {
		ASSERT_TEST(is_SHORT(getpid()) == 1);
		ASSERT_TEST(remaining_time(getpid()) <= 1500);
		ASSERT_TEST(remaining_cooloffs(getpid()) == 2);
		exit(0);
    } 
	waitpid(pid, &status, 0);
	return true;
		
}


int main() {
	RUN_TEST(shortFork);
	return 0;
}

