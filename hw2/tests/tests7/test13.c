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

bool systemCallsTest() {

	struct sched_param_ex sp;
	sp.sched_priority = 0;
	sp.requested_time = 500;
	sp.num_cooloff = 1;
	ASSERT_TEST(is_SHORT(getpid()) == -1); //we aren't short nor overdue
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(is_SHORT(-5) == -1); //there isn't such a process
	ASSERT_TEST(errno == EINVAL); //according to piazza if no process is found we should return EINVAL
	ASSERT_TEST(remaining_time(getpid()) == -1);
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(remaining_time(-300) == -1);
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(remaining_cooloffs(getpid()) == -1);
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(remaining_cooloffs(-getpid()) == -1);
	ASSERT_TEST(errno == EINVAL);
	sched_setscheduler(getpid(), SCHED_SHORT, (struct sched_param *)&sp); // becomes a SHORT
	ASSERT_TEST(is_SHORT(getpid()) == 1);
	ASSERT_TEST(remaining_time(getpid())>0 && remaining_time(getpid()) <=500); 
	ASSERT_TEST(remaining_cooloffs(getpid()) == 1); //we assume we are still regular SHORT here
	busywait(520); //we should become overdue here
	ASSERT_TEST(is_SHORT(getpid()) == 0);
	ASSERT_TEST(remaining_time(getpid())>0 && remaining_time(getpid()) <=500);
	ASSERT_TEST(remaining_cooloffs(getpid()) == 0); //although we havent returned from cooloff this should return 0 now
	busywait(520); //we should become regular SHORT here
	ASSERT_TEST(is_SHORT(getpid()) == 1);
	ASSERT_TEST(remaining_time(getpid())>0 && remaining_time(getpid()) <=500);
	ASSERT_TEST(remaining_cooloffs(getpid()) == 0); 
	busywait(520); //we should become overdue here, for good
	ASSERT_TEST(is_SHORT(getpid()) == 0);
	printf("%d\n", remaining_time(getpid()));
	ASSERT_TEST(remaining_time(getpid())==0); //if we are overdue for good this should return 0
	ASSERT_TEST(remaining_cooloffs(getpid()) == 0);
	return true;
	
	
		
}


int main() {
	RUN_TEST(systemCallsTest);

	
	return 0;
}