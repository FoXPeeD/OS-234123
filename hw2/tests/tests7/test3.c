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


bool setscheduleTest() {
	struct sched_param_ex sp;
	sp.sched_priority = 5;
	sp.requested_time = 200;
	sp.num_cooloff = 3;
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_RR, (struct sched_param *)&sp),0);
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp),-1);
	ASSERT_EQUALS(errno,EPERM);
	sp.sched_priority = -5;
	sp.requested_time = -200;
	sp.num_cooloff = -3;
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp),-1);
	ASSERT_EQUALS(errno,EPERM);
	sp.sched_priority = 0;
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_OTHER, (struct sched_param *)&sp),0);
	
	sp.requested_time = 0;
	sp.num_cooloff = 3;
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp),-1);
	ASSERT_EQUALS(errno,EINVAL);
//last
	sp.requested_time = 500;
	sp.num_cooloff = 6;
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp),-1);
	ASSERT_EQUALS(errno,EINVAL);
//last
	sp.sched_priority = -5000;
	sp.requested_time = 501;
	sp.num_cooloff = 3;
	struct sched_param_ex sp1;
	
	printf("pid is: %d\n", getpid());
	
	ASSERT_EQUALS(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp), 0);
	/*
	ASSERT_TEST(remaining_cooloffs(getpid()),3);
	ASSERT_TEST(sched_getparam(getpid(),(struct sched_param *)&sp1),0); 
	ASSERT_TEST(sp1.requested_time, 501);
	ASSERT_TEST(is_SHORT(getpid()));
	ASSERT_TEST(sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp),-1);
	ASSERT_TEST(errno,EPERM);
	ASSERT_TEST(sched_setscheduler(getpid(), SCHED_OTHER, (struct sched_param *)&sp),-1);
	ASSERT_TEST(errno,EPERM);
	*/
	return true;
}

int main() {
	RUN_TEST(setscheduleTest);
	return 0;
}