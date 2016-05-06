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

bool getSetParamTest() {
	pid_t pid;
	struct sched_param_ex sp;
	int status;
	sp.sched_priority = 0;
	sp.requested_time = 1000;
	sp.num_cooloff = 3;
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); // becomes a SHORT
	sp.requested_time = 3500;
	ASSERT_TEST(sched_setparam(getpid(), (struct sched_param *)&sp) == -1); //requested time is not valid
	ASSERT_TEST(errno == EINVAL);
	sp.requested_time = 1500;
	sp.num_cooloff = -3;
	sp.sched_priority = -20;
	ASSERT_TEST(sched_setparam(getpid(), (struct sched_param *)&sp) == 0); //requested time is valid, should ignore other paramaters according to piazza
	ASSERT_TEST(sched_setparam(getpid(), (struct sched_param *)&sp) == 0); //changing to the same time is not an error
	while (is_SHORT(getpid()) == 1) {} // we wait until we become overdue
	ASSERT_TEST(remaining_time(getpid()) <= 1000); //we should not change cooloff time to 1500 here just yet, when we are overdue, only when we become regular SHORT
	while (is_SHORT(getpid()) == 0) {} //we wait until we become regular short
	ASSERT_TEST(remaining_time(getpid()) > 1000); //now our remaining time should be approx 1500
	ASSERT_TEST(sched_getparam(getpid(),(struct sched_param *)&sp) == 0);
	ASSERT_TEST(sp.num_cooloff == 3); //although we have wasted 1 cooloff, according to piazza getparam returns original cooloff
	ASSERT_TEST(sp.requested_time == 1500); //this should return our requested time
	sp.requested_time = 200;
	ASSERT_TEST(sched_setparam(getpid(), (struct sched_param *)&sp) == 0);
	ASSERT_TEST(sched_getparam(getpid(),(struct sched_param *)&sp) == 0);
	printf("%d\n", sp.requested_time);
	ASSERT_TEST(sp.requested_time == 200); //although we dont run for 200 just yet, only when we return from cooloff, this should still return 200 according to piazza
	sp.requested_time = 300;
	ASSERT_TEST(sched_setparam(getpid(), (struct sched_param *)&sp) == 0);
	ASSERT_TEST(sched_getparam(getpid(),(struct sched_param *)&sp) == 0);
	ASSERT_TEST(sp.requested_time == 300); //same as before
	pid = fork(); 
	if(pid ==0) { //son
		while(is_SHORT(getpid()) == 1) {} //we assume that father was still regular SHORT before fork. we wait for son to become overdue
		ASSERT_TEST(remaining_time(getpid()) > 1000); // the overdue son should have 1500ms cooloff period
		ASSERT_TEST(sched_getparam(getpid(),(struct sched_param *)&sp) == 0);
		ASSERT_TEST(sp.requested_time == 300); //this should be like fathers
		while(is_SHORT(getpid()) == 0) {} //we wait for son to become regular SHORT
		ASSERT_TEST(remaining_time(getpid()) <= 300); //father has new requested time of 300, son should change here as well
		exit(0);
	}
	waitpid(pid, &status, 0);
	return true;
	
	
		
}


int main() {
	RUN_TEST(getSetParamTest);
	return 0;
}