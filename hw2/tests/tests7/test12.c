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

#define NANO_SECOND_MULTIPLIER  1000000  // 1 millisecond = 1,000,000 Nanoseconds




// THIS TEST IS PROBLEMTAC! IF WE IMPLEMENT THE RULE WHERE SHORT/OVERDUE GO TO LAST PLACE IF THEY ARE SWITCHED OFF CPU, THIS TEST FAILS
// THE OUTPUT NEEDS TO BE: 1 2 3 4 5 6 7 8 9

bool shortVsOverdue() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	struct timespec sleepValue = {0};
	
	sp.sched_priority = 0;
	sp.requested_time = 100;
	sp.num_cooloff = 1;
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); //father becomes a SHORT
	pid = fork();  //son has 1 cooloff remaining and father has 0. father has less than 50ms remaining and so does son
	if(pid != 0) {
		printf("2\n"); //son is overdue so father should run
		busywait(60); //father becomes overdue here
		printf("4\n"); // son is asleep so father should run
		busywait(130); //father is still overdue (has 0 cooloff) while SHORT son wakes up here
		printf("6\n"); //overdue is FIFO and father became overdue first
		sleepValue.tv_nsec = 120 * NANO_SECOND_MULTIPLIER;
		nanosleep(&sleepValue,NULL); //father sleeps for 120ms
		printf("9\n"); //son has died, father should run
	}
	else {
        // Child, a SHORT.
		printf("1\n");  // after SHORT forks, son should run first
		busywait(60); //son should become overdue here
		printf("3\n"); // overdue is FIFO, and since son became overdue first, he should run now
		busywait(120); //son becomes SHORT again
		sleepValue.tv_nsec = 110 * NANO_SECOND_MULTIPLIER;
		nanosleep(&sleepValue,NULL); //son sleeps for 110ms
		printf("5\n"); //son is SHORT while father is OVERDUE
		busywait(120); //son becomes overdue again
		printf("7\n");
		busywait(300); //father is still overdue, son should still run
		printf("8\n");
		exit(0);
    }
	waitpid(pid, &status, 0);
	return true;
		
}


int main() {
	printf("shortVsOverdue output:\n");
	shortVsOverdue();
	return 0;
}	
