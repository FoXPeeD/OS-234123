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
{
    struct timespec tstart, tend;
    long int runtime;

    clock_gettime(CLOCK_REALTIME, &tstart);
    for(;;) {
        clock_gettime(CLOCK_REALTIME, &tend);
        runtime = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
        if(runtime >= ms) {
            return;
        }
    }
}


// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6 7

bool shortVsBetterShort() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	sp.sched_priority = 0;
	sp.requested_time = 500;
	sp.num_cooloff = 3;
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); //father becomes a SHORT
	pid = fork();  
	if(pid != 0) {
		printf("2\n"); // son is asleep and father should runs
		nice(1); //father becomes slightly worse than son
		busywait(120); //son should wake up by now, he is a better SHORT
		printf("4\n"); //son is overdue so father should run
		usleep(520000);// father sleeps for 500ms
		printf("7\n"); //son is dead so father should run	
	}
	else {
        // Child, a SHORT.
		printf("1\n"); //after fork of SHORT child runs first
		usleep(100000); // son sleeps for 100ms, should be enough for father to complete nice()
		printf("3\n"); //son is a better SHORT so he should run now
		busywait(300); //son should become overdue now
		printf("5\n"); // father is asleep so overdue son should run
		busywait(500); //son should become SHORT again now
		busywait(200); //father have sure woken up by now but son is a better SHORT
		printf("6\n");
		exit(0);
    } 
	waitpid(pid, &status, 0);
	return true;
		
}


int main() {
	printf("shortVsBetterShort output:\n");
	shortVsBetterShort();
	return 0;
}	