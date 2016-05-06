#include  <stdio.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <sys/types.h>
#include "ctx_log_test_utilities.h"
#include "ctx_log_wrappers.h"
#include "hw2_syscalls.h"
#include "test_utilities.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct sched_param {
    int sched_priority;
    int requested_time;
    int number_of_cooloffs;
} sched_param_t;

#define MS_BETWEEN_CLOCKS(c1,c2) (((double) ((c2) - (c1))) / (CLOCKS_PER_SEC/1000))
#define FILE_PATH "dummy.txt"
#define O_RDONLY 00
#define O_CREAT 0100
#define O_WRONLY 01
#define O_TRUNC 01000
static void busyWait(int delay_milliseconds){
	clock_t start,end;
	start = clock();
	end = clock();
	while (MS_BETWEEN_CLOCKS(start,end) < delay_milliseconds)
	{
		end = clock();
	}
}

// Doesn't work for some reason (seems not even going to wait), checking with raw log....
static void goToWaitQueue(){
	int fd = open(FILE_PATH,O_CREAT | O_WRONLY | O_TRUNC);
	write(fd, "Hello\n\n.\n", 9);
	fsync(fd);
	close(fd);
}

#define PRIO_RT 55
#define PRIO_OTHER 0 // see Linus's comment on setscheduler (when changin g to OTHER must be 0)
#define PRIO_SHORT 20 // because of wierd != condition it mustnt be 0;
#define MIN_REQUESTED_TIME 1
#define MAX_REQUESTED_TIME 3000
#define MIN_COOLOFF_CYCLES 0
#define MAX_COOLOFF_CYCLES 5

static int make_me_rt_with_policy(int policy){
	sched_param_t params = {PRIO_RT,MAX_REQUESTED_TIME,MIN_COOLOFF_CYCLES};
	return sched_setscheduler(getpid(), policy, &params);
}
static int make_me_rt(){
	return make_me_rt_with_policy(SCHED_FIFO);
}
static int make_me_other(){
	sched_param_t params = {PRIO_OTHER,MAX_REQUESTED_TIME,MIN_COOLOFF_CYCLES};
	int result = sched_setscheduler(getpid(), SCHED_OTHER, &params);
	return result;
}
static int make_me_short(){
	sched_param_t params = {PRIO_OTHER,MAX_REQUESTED_TIME,MIN_COOLOFF_CYCLES};
	int result = sched_setscheduler(getpid(), PRIO_SHORT, &params);
	return result;
}
static int make_rt_child_become_short_with_params(pid_t child,int req_time,int cooloff){
	sched_param_t params = {PRIO_OTHER,req_time,cooloff};
	if(!(sched_setscheduler(child, SCHED_OTHER, &params)==0)){
		return -1;
	}
	params.sched_priority = PRIO_SHORT;
	int result = sched_setscheduler(child, SCHED_SHORT, &params);
	return result;
}
static int make_rt_child_become_short(pid_t child){
	return make_rt_child_become_short_with_params(child,MAX_REQUESTED_TIME,MIN_COOLOFF_CYCLES);
}
static int make_rt_child_become_super_short(pid_t child){
	return make_rt_child_become_short_with_params(child,100,MIN_COOLOFF_CYCLES);
}

#define LOG_SIZE CTX_LOG_MAX_SIZE

/// TODO ::: WHAT ABOUT ALL SON PROCS? SHOULD I DO A WAIT LOOP AT THE END OF EACH TEST TO LET THEM DIE IN PEACE?
/*************************************************************************

========= TESTS INVOLVING THE INSIDE SCHEDULE MECHANISM ============

*************************************************************************/
ctx_idx_t last_short_obtained_ctx_id;
//////
////// tests on SHORTS that are initialized together and run to completion
//////
static bool testSimpleShorts_superSimple(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());

	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	return true;
}

static bool testSimpleShorts_BetterPrioRunsFirst(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());

	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield();
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		exit(11);
	}
	
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* first_short = find_next_rec_short(log,log_actual_size);
    ASSERT(first_short);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	
	printf("child1=%d, child2=%d, child3=%d\n", child1, child2, child3);
	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}



static bool testSimpleShorts_SamePrioRunInFIFO(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child5 = fork();
	if(child5==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child5));
		sched_yield();
		exit(55);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield();
		exit(22);
	}
	pid_t child3 = fork();
	if(child3==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child4 = fork();
	if(child4==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child4));
		sched_yield();
		exit(44);
	}
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		exit(11);
	}
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);
	req_find_next_rec_short_after_specific(fifth_short,log,log_actual_size,fourth_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child3);
	ASSERT_EQUALS((fourth_short->next_info).pid,child4);
	ASSERT_EQUALS((fifth_short->prev_info).pid,child4);
	ASSERT_EQUALS((fifth_short->next_info).pid,child5);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
static bool testSimpleShorts_BestBecomesWorst_ImmediateSwitchAndRunInNewOrder(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield();
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		nice(5);
		exit(11);
	}
	
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child3);
	ASSERT_EQUALS((fourth_short->next_info).pid,child1);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

//////
////// tests on SHORTS with waiting involved
//////
static bool testWaitingShorts_BestWaits_ComesBackToFirst(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield();
		//busyWait(1000); // we first run when child1 waits, we want to keep running when it comes  back so it can cut us off.
        complete_debug_work(); // we should expect to loss control here to child1.
        busyWait(20);
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		// went to yield as RT, came back as short. we run first of all the shorts
		
		// Doesn't work for some reason (seems not even going to wait), checking with raw log....
		//goToWaitQueue(); // here we go to wait for a tiny bit and then come back, we should immediately get processor time.
        
        wait_for_completion_of_debug_work();
		exit(11);
	}
	
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);
	req_find_next_rec_short_after_specific(fifth_short,log,log_actual_size,fourth_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child1);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child1);
	ASSERT_EQUALS((fourth_short->next_info).pid,child2);
	ASSERT_EQUALS((fifth_short->prev_info).pid,child2);
	ASSERT_EQUALS((fifth_short->next_info).pid,child3);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
static bool testWaitingShorts_SamePrioWaits_ComesBackToLastInPrio(){
	return true;
}
static bool testWaitingShorts_BestWaits_LesserGetsNiceOfBest_BestComesBackBehindIt(){
	return true;
}
static bool testWaitingShorts_RTWaits_ComesBackToFirst(){
	return true;
}

//////
////// tests on SHORTS with yielding involved
//////
static bool testYieldingShorts_SingleBestYields_NoSwitch(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield();
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		sched_yield(); // yields as best SHORT, should "switch" to itself
		exit(11);
	}
	
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	
	// second_short is a switch to self
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child1);
	
	ASSERT_EQUALS((third_short->prev_info).pid,child1);
	ASSERT_EQUALS((third_short->next_info).pid,child2);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child2);
	ASSERT_EQUALS((fourth_short->next_info).pid,child3);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
static bool testYieldingShorts_OneOfBestsYields_GoesBackToEndOfHisPrio(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child5 = fork();
	if(child5==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child5));
		sched_yield();
		exit(55);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		sched_yield(); // yields as best SHORT, should go to back of line
		exit(22);
	}
	pid_t child3 = fork();
	if(child3==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child4 = fork();
	if(child4==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child4));
		sched_yield();
		exit(44);
	}
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		exit(11);
	}
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);
	req_find_next_rec_short_after_specific(fifth_short,log,log_actual_size,fourth_short);
	req_find_next_rec_short_after_specific(sixth_short,log,log_actual_size,fifth_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child3);
	ASSERT_EQUALS((fourth_short->next_info).pid,child4);
	ASSERT_EQUALS((fifth_short->prev_info).pid,child4);
	ASSERT_EQUALS((fifth_short->next_info).pid,child2);
	ASSERT_EQUALS((sixth_short->prev_info).pid,child2);
	ASSERT_EQUALS((sixth_short->next_info).pid,child5);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

//////
////// tests on SHORTS with forking involved
//////
static bool testForkingShorts_TwoProcs_ChildRunsFirst(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t father = fork();
	pid_t child = father+1; // stupid to do... might fail...
	if(father==0){
		ASSERT_ZERO(make_rt_child_become_short(father));
		sched_yield();
		fork();
		exit(55);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
    
	ASSERT_EQUALS((first_short->next_info).pid,father);
	ASSERT_EQUALS((second_short->prev_info).pid,father);
	ASSERT_EQUALS((second_short->next_info).pid,child);
	ASSERT_EQUALS((third_short->prev_info).pid,child);
	ASSERT_EQUALS((third_short->next_info).pid,father);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
static bool testForkingShorts_TwoProcs_ChildBecomesNicer_SwitchBackToParent(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t father = fork();
	pid_t child = father+1; // stupid to do... might fail...
	if(father==0){
		ASSERT_ZERO(make_rt_child_become_short(father));
		sched_yield();
		if(fork()==0){
			nice(1);
		}
		else{
			nice(2);
		}
		exit(55);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);
	req_find_next_rec_short_after_specific(fifth_short,log,log_actual_size,fourth_short);

	ASSERT_EQUALS((first_short->next_info).pid,father);
	ASSERT_EQUALS((second_short->prev_info).pid,father);
	ASSERT_EQUALS((second_short->next_info).pid,child);
	ASSERT_EQUALS((third_short->prev_info).pid,child);
	ASSERT_EQUALS((third_short->next_info).pid,father);
	ASSERT_EQUALS((fourth_short->prev_info).pid,father);
	ASSERT_EQUALS((fourth_short->next_info).pid,child);
	ASSERT_EQUALS((fifth_short->prev_info).pid,child);
	ASSERT_EQUALS((fifth_short->next_info).pid,father);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

// Based on Piazza question 222,
// https://piazza.com/class/iliabemchdwbj?cid=222
static bool testForkingShorts_MoreProcs_ChildRunsFirst_ParentRunsLastInPrio(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child5 = fork();
	if(child5==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_short(child5));
		sched_yield();
		exit(55);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child2));
		sched_yield(); // yielding as RT, coming back as SHORT
		pid_t child_of_2 = fork();
		if(child_of_2==0){
			exit(22);
		}
		exit(child_of_2); // does not work, exit status is limited to 0-255...
	}
	pid_t child3 = fork();
	if(child3==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child3));
		sched_yield();
		exit(33);
	}
	pid_t child4 = fork();
	if(child4==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_short(child4));
		sched_yield();
		exit(44);
	}
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield();
		exit(11);
	}
	// All short are ready to run, now I lower my prio and they run in order,
	// then I get control back and check what happened while I was off
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// Here I am back after all shorts did run
	pid_t child_of_2 = child1+1; // stupid, should find another mechanism
	ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);
	req_find_next_rec_short_after_specific(fifth_short,log,log_actual_size,fourth_short);
	req_find_next_rec_short_after_specific(sixth_short,log,log_actual_size,fifth_short);
	req_find_next_rec_short_after_specific(seventh_short,log,log_actual_size,sixth_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child_of_2);
	ASSERT_EQUALS((fourth_short->prev_info).pid,child_of_2);
	ASSERT_EQUALS((fourth_short->next_info).pid,child3);
	ASSERT_EQUALS((fifth_short->prev_info).pid,child3);
	ASSERT_EQUALS((fifth_short->next_info).pid,child4);
	ASSERT_EQUALS((sixth_short->prev_info).pid,child4);
	ASSERT_EQUALS((sixth_short->next_info).pid,child2);
	ASSERT_EQUALS((seventh_short->prev_info).pid,child2);
	ASSERT_EQUALS((seventh_short->next_info).pid,child5);
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
//////
////// simple tests on OVERDUE-SHORTS
//////
static bool testSimpleOverdueShorts_ExecuteFIFO(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
	ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_super_short(child3));
		sched_yield();
		busyWait(1000);
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_super_short(child2));
		sched_yield();
		busyWait(1000);
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_super_short(child1));
		sched_yield();
		busyWait(1000);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// After this all shorts are run and already in overdue, but didn't start running.
	
	int status;
	waitpid(child3,&status,0); // That's what to see  whats up... it not good enough because one of the overdues has to release us and give up CPU...
	// here all overdues are supposed to run (DEPENDS ON OTHER "OTHERS" IN THE SYSTEM, BUT FROM OUR EXPERIENCE THEY ALL WAIT AT SOME POINT AND THE OVERDUES DO RUN)
	
		ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);
	ASSERT_EQUALS((fourth_short->next_info).pid,child1);
	ASSERT((fourth_short->next_info).is_overdue_forever);
	// If this works nice, extract it to func/macro
	pid_t current_pid = (fourth_short->next_info).pid;
	ctx_log_record_t *current_overdue = fourth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *fifth_short = current_overdue;
	current_pid = (fifth_short->next_info).pid;
	current_overdue = fifth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *sixth_short = current_overdue;
	current_pid = (sixth_short->next_info).pid;
	current_overdue = sixth_short;
	
	ASSERT_EQUALS((fifth_short->next_info).pid,child2);
	ASSERT((fifth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((sixth_short->next_info).pid,child3);
	ASSERT((sixth_short->next_info).is_overdue_forever);
	
	/******************************************************************
	*******************************************************************
	*******************************************************************
	
	With the "new" overdue-swithced-off-goes-to-back-of-line we need complicated ASSERTS here,
	serious shit.
    
    >>> Correct. they can be found in verify_ctx_log()
        so that here we only have to verify that the process has records:
            1. when it got into overdue for the first time.
            2. when it exited.
        and than we would know it behaved exactly like we want.
        Also, this issue (and more) can pass in this specific Unit-Test but fail when another things are involed.
            That is because we are humen, and humen cannot know when/where they code would fail.
            They barly can undersdand why it has been failed, even when you tell them the exact file&line it crashed.
            If we could predict it we won't need tests to tell us this.
	
	*******************************************************************
	*******************************************************************
	******************************************************************/
	
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}
static bool testSimpleOverdueShorts_SwitchedOffForAnyRT(){
	return true;
}
static bool testSimpleOverdueShorts_SwitchedOffForAnySHORT(){
	return true;
}
static bool testSimpleOverdueShorts_SwitchedOffForAnyOTHER(){
	return true;
}

//////
////// tests on OVERDUE-SHORTS with waiting involved
//////
static bool testWaitingOverdueShorts_GoesToBackOfLine(){
		get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_super_short(child3));
		sched_yield();
		busyWait(1000);
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_super_short(child2));
		sched_yield();
		busyWait(1000);
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_super_short(child1));
		sched_yield();
		busyWait(1000);
		usleep(50*1000);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// After this all shorts are run and already in overdue, but didn't start running.
	
	int status;
	waitpid(child3,&status,0);
	sleep(1);
	// here all overdues are supposed to run (DEPENDS ON OTHER "OTHERS" IN THE SYSTEM, BUT FROM OUR EXPERIENCE THEY ALL WAIT AT SOME POINT AND THE OVERDUES DO RUN)
	
		ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);

	// If this works nice, extract it to func/macro
	pid_t current_pid = (fourth_short->next_info).pid;
	ctx_log_record_t *current_overdue = fourth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *fifth_short = current_overdue;
	current_pid = (fifth_short->next_info).pid;
	current_overdue = fifth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *sixth_short = current_overdue;
	current_pid = (sixth_short->next_info).pid;
	current_overdue = sixth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *seventh_short = current_overdue;
	
	/*
	The order should be:
	1
	2
	3
	1
	*/
	
	ASSERT_EQUALS((fourth_short->next_info).pid,child1);
	ASSERT((fourth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((fifth_short->next_info).pid,child2);
	ASSERT((fifth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((sixth_short->next_info).pid,child3);
	ASSERT((sixth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((seventh_short->next_info).pid,child1);
	ASSERT((seventh_short->next_info).is_overdue_forever);
	
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

//////
////// tests on OVERDUE-SHORTS with forking involved
//////
/// TODO :: Based on Piazza @222
static bool testForkingOverdueShorts_ParentRuns_ChildGoesToBackOfLine(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_super_short(child3));
		sched_yield();
		busyWait(1000);
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_super_short(child2));
		sched_yield();
		busyWait(1000);
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_super_short(child1));
		sched_yield();
		busyWait(1000);
		fork(); // TODO :: Use shared memory to return pid of child
		busyWait(100);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// After this all shorts are run and already in overdue, but didn't start running.
	
	int status;
	waitpid(child3,&status,0);
	sleep(1);
	// here all overdues are supposed to run (DEPENDS ON OTHER "OTHERS" IN THE SYSTEM, BUT FROM OUR EXPERIENCE THEY ALL WAIT AT SOME POINT AND THE OVERDUES DO RUN)
	
		ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);

	// If this works nice, extract it to func/macro
	pid_t current_pid = (fourth_short->next_info).pid;
	ctx_log_record_t *current_overdue = fourth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *fifth_short = current_overdue;
	current_pid = (fifth_short->next_info).pid;
	current_overdue = fifth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *sixth_short = current_overdue;
	current_pid = (sixth_short->next_info).pid;
	current_overdue = sixth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *seventh_short = current_overdue;
	
	/*
	The order should be:
	1
	2
	3
	1' (his fork child)
	*/
	
	ASSERT_EQUALS((fourth_short->next_info).pid,child1);
	ASSERT((fourth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((fifth_short->next_info).pid,child2);
	ASSERT((fifth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((sixth_short->next_info).pid,child3);
	ASSERT((sixth_short->next_info).is_overdue_forever);
	
	// TODO :: pass 1' pid in shared mem and check for it explicitly
	// in the mean time, check NQ to every other short.
	ASSERT_NQ((seventh_short->next_info).pid,child1);
	ASSERT_NQ((seventh_short->next_info).pid,child2);
	ASSERT_NQ((seventh_short->next_info).pid,child3);
	ASSERT((seventh_short->next_info).is_overdue_forever);
	
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

//////
////// tests on OVERDUE-SHORTS with yielding involved
//////
static bool testYieldingOverdueShorts_GoesToBackOfLine(){
	get_last_ctx_id(&last_short_obtained_ctx_id);
		ASSERT_ZERO(make_me_rt());
	
	// Creating 3 short procs, I am RT so they don't run yet.
	pid_t child3 = fork();
	if(child3==0){
		nice(3);
		ASSERT_ZERO(make_rt_child_become_super_short(child3));
		sched_yield();
		busyWait(1000);
		exit(33);
	}
	pid_t child2 = fork();
	if(child2==0){
		nice(2);
		ASSERT_ZERO(make_rt_child_become_super_short(child2));
		sched_yield();
		busyWait(1000);
		exit(22);
	}
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_super_short(child1));
		sched_yield();
		busyWait(1000);
		sched_yield();
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	// After this all shorts are run and already in overdue, but didn't start running.
	
	int status;
	waitpid(child3,&status,0);
	sleep(1);
	// here all overdues are supposed to run (DEPENDS ON OTHER "OTHERS" IN THE SYSTEM, BUT FROM OUR EXPERIENCE THEY ALL WAIT AT SOME POINT AND THE OVERDUES DO RUN)
	
		ctx_log_record_t *log = malloc(sizeof(ctx_log_record_t)*LOG_SIZE);
    ASSERT(log);
	int log_actual_size = get_ctx_log(log,LOG_SIZE);
    WRITE_LOG_TO_FILE(log, log_actual_size); // to see run `showlog <testname>.log`
    ASSERT(verify_ctx_log(log,log_actual_size)); // TODO: activate [remove remark].
	ctx_log_record_t* prev = get_rec_by_ctx_id(log,log_actual_size,last_short_obtained_ctx_id);
    ASSERT(prev);
	req_find_next_rec_short_after_specific(first_short,log,log_actual_size,prev);
	req_find_next_rec_short_after_specific(second_short,log,log_actual_size,first_short);
	req_find_next_rec_short_after_specific(third_short,log,log_actual_size,second_short);
	req_find_next_rec_short_after_specific(fourth_short,log,log_actual_size,third_short);

	ASSERT_EQUALS((first_short->next_info).pid,child1);
	ASSERT_EQUALS((second_short->prev_info).pid,child1);
	ASSERT_EQUALS((second_short->next_info).pid,child2);
	ASSERT_EQUALS((third_short->prev_info).pid,child2);
	ASSERT_EQUALS((third_short->next_info).pid,child3);

	// If this works nice, extract it to func/macro
	pid_t current_pid = (fourth_short->next_info).pid;
	ctx_log_record_t *current_overdue = fourth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *fifth_short = current_overdue;
	current_pid = (fifth_short->next_info).pid;
	current_overdue = fifth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *sixth_short = current_overdue;
	current_pid = (sixth_short->next_info).pid;
	current_overdue = sixth_short;
	while((current_overdue->next_info).pid == current_pid){
		current_overdue = find_next_rec_short_after_specific(log,log_actual_size,current_overdue);
	}
	ctx_log_record_t *seventh_short = current_overdue;
	
	/*
	The order should be:
	1
	2
	3
	1
	*/
	
	ASSERT_EQUALS((fourth_short->next_info).pid,child1);
	ASSERT((fourth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((fifth_short->next_info).pid,child2);
	ASSERT((fifth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((sixth_short->next_info).pid,child3);
	ASSERT((sixth_short->next_info).is_overdue_forever);
	ASSERT_EQUALS((seventh_short->next_info).pid,child1);
	ASSERT((seventh_short->next_info).is_overdue_forever);
	
	// TODO :: Add more and more ASSERT_* for every relevant piece of data in the log
	free(log);
	return true;
}

/*************************************************************************

=========== TESTS INVOLVING THE OUTSIDE API (SYS_CALLS) ==============

*************************************************************************/

//////
////// tests on is_SHORT
//////
static bool testIsShort_Invalid_OTHER(){
	ASSERT_ZERO(make_me_rt());
	ASSERT_ZERO(make_me_other());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_OTHER);
	ASSERT_EQUALS(is_SHORT(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testIsShort_Invalid_FIFO(){
	ASSERT_ZERO(make_me_rt());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_FIFO);
	ASSERT_EQUALS(is_SHORT(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testIsShort_Invalid_RR(){
	ASSERT_ZERO(make_me_rt_with_policy(SCHED_RR));
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_RR);
	ASSERT_EQUALS(is_SHORT(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testIsShort_Invalid_NotExistPID(){
	ASSERT_EQUALS(is_SHORT(1000*1000),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testIsShort_Invalid_NotLegalPID(){
	ASSERT_EQUALS(is_SHORT(-1),-1);
	ASSERT_EQUALS(EINVAL,errno);
	ASSERT_EQUALS(is_SHORT(-5),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testIsShort_Valid_RegularShort(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		exit(11);
	}
	
	ASSERT_EQUALS(is_SHORT(child1),-1);
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	ASSERT_EQUALS(is_SHORT(child1),1);
	return true;
}
static bool testIsShort_Valid_RegularShortAfterFork(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short(child1));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	ASSERT_EQUALS(is_SHORT(child1),-1); // not yet short
	ASSERT_EQUALS(is_SHORT(child_of_1),-1); // still not exists
	ASSERT_EQUALS(EINVAL,errno);
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(is_SHORT(child1),1);
	ASSERT_EQUALS(is_SHORT(child_of_1),1);
	return true;
}
static bool testIsShort_Valid_OverdueShort(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_super_short(child1));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(1000);
		exit(11);
	}
	
	ASSERT_EQUALS(is_SHORT(child1),-1);
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	ASSERT_EQUALS(is_SHORT(child1),0); // here I am back and it is overdue
	return true;
}

//////
////// tests on remaining_time
//////
static bool testRemTime_Invalid_OTHER(){
	ASSERT_ZERO(make_me_rt());
	ASSERT_ZERO(make_me_other());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_OTHER);
	ASSERT_EQUALS(remaining_time(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemTime_Invalid_FIFO(){
	ASSERT_ZERO(make_me_rt());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_FIFO);
	ASSERT_EQUALS(remaining_time(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemTime_Invalid_RR(){
	ASSERT_ZERO(make_me_rt_with_policy(SCHED_RR));
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_RR);
	ASSERT_EQUALS(remaining_time(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemTime_Invalid_NotExistPID(){
	ASSERT_EQUALS(remaining_time(1000*1000),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemTime_Invalid_NotLegalPID(){
	ASSERT_EQUALS(remaining_time(-1),-1);
	ASSERT_EQUALS(EINVAL,errno);
	ASSERT_EQUALS(remaining_time(-5),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemTime_Valid_Short(){
	return true;
}
static bool testRemTime_Valid_ShortAfterFork_HalfTime(){
	return true;
}
static bool testRemTime_Valid_Overdue(){
	return true;
}
static bool testRemTime_Valid_OverdueAfterFork_OriginalTime(){
	return true;
}
static bool testRemTime_Valid_OverdueForever_0(){
	return true;
}
static bool testRemTime_Valid_OverdueForeverAfterFork_0(){
	return true;
}

//////
////// tests on remaining_cooloffs
//////
static bool testRemCooloff_Invalid_OTHER(){
	ASSERT_ZERO(make_me_rt());
	ASSERT_ZERO(make_me_other());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_OTHER);
	ASSERT_EQUALS(remaining_cooloffs(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemCooloff_Invalid_FIFO(){
	ASSERT_ZERO(make_me_rt());
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_FIFO);
	ASSERT_EQUALS(remaining_cooloffs(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemCooloff_Invalid_RR(){
	ASSERT_ZERO(make_me_rt_with_policy(SCHED_RR));
	sched_yield();
	ASSERT_EQUALS(sched_getscheduler(getpid()),SCHED_RR);
	ASSERT_EQUALS(remaining_cooloffs(getpid()),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemCooloff_Invalid_NotExistPID(){
	ASSERT_EQUALS(remaining_cooloffs(1000*1000),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemCooloff_Invalid_NotLegalPID(){
	ASSERT_EQUALS(remaining_cooloffs(-1),-1);
	ASSERT_EQUALS(EINVAL,errno);
	ASSERT_EQUALS(remaining_cooloffs(-5),-1);
	ASSERT_EQUALS(EINVAL,errno);
	return true;
}
static bool testRemCooloff_Valid_Short(){
	return true;
}
static bool testRemCooloff_Valid_ShortAfterFork_HalfNumber_OddOriginal(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,MAX_REQUESTED_TIME,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	ASSERT_EQUALS(is_SHORT(child1),-1); // not yet short
	ASSERT_EQUALS(is_SHORT(child_of_1),-1); // still not exists
	ASSERT_EQUALS(EINVAL,errno);
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(remaining_cooloffs(child1),2);
	ASSERT_EQUALS(remaining_cooloffs(child_of_1),3);
	return true;
}
static bool testRemCooloff_Valid_ShortAfterFork_HalfNumber_EvenOriginal(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,MAX_REQUESTED_TIME,4));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	ASSERT_EQUALS(is_SHORT(child1),-1); // not yet short
	ASSERT_EQUALS(is_SHORT(child_of_1),-1); // still not exists
	ASSERT_EQUALS(EINVAL,errno);
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(remaining_cooloffs(child1),2);
	ASSERT_EQUALS(remaining_cooloffs(child_of_1),2);
	return true;
}
static bool testRemCooloff_Valid_ShortAfterFork_HalfNumber_SingleOriginal(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,MAX_REQUESTED_TIME,1));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	ASSERT_EQUALS(is_SHORT(child1),-1); // not yet short
	ASSERT_EQUALS(is_SHORT(child_of_1),-1); // still not exists
	ASSERT_EQUALS(EINVAL,errno);
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(remaining_cooloffs(child1),0);
	ASSERT_EQUALS(remaining_cooloffs(child_of_1),1);
	return true;
}
static bool testRemCooloff_Valid_ShortAfterFork_HalfNumber_ZeroOriginal(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,MAX_REQUESTED_TIME,0));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	ASSERT_EQUALS(is_SHORT(child1),-1); // not yet short
	ASSERT_EQUALS(is_SHORT(child_of_1),-1); // still not exists
	ASSERT_EQUALS(EINVAL,errno);
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(remaining_cooloffs(child1),0);
	ASSERT_EQUALS(remaining_cooloffs(child_of_1),0);
	return true;
}
static bool testRemCooloff_Valid_Overdue_ShouldAlreadyBeLess(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		wait_for_completion_of_debug_work();
		busyWait(1000);
		exit(11);
	}
	
	ASSERT_EQUALS(is_SHORT(child1),-1);
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	/*
	what will happen here
	child will run as short
	child will wait for completion
	we check that cooloffs is full
	we complete
	the child goes to overdue, we get back control
	we check that we already have 1 less
	*/
	ASSERT_EQUALS(remaining_cooloffs(child1),5);
	complete_debug_work();
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	return true;
}
static bool testRemCooloff_Valid_OverdueAfterFork_HalfNumber(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,4));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(120);
		pid_t child = fork();
		if(child != 0) complete_debug_work();
		exit(11);
	}
	pid_t child_of_1 = child1+1;
	
	wait_for_completion_of_debug_work();
	// Here I am back after all shorts did run
	ASSERT_EQUALS(is_SHORT(child1),0);
	ASSERT_EQUALS(is_SHORT(child_of_1),0);
	ASSERT_EQUALS(remaining_cooloffs(child1),1);
	ASSERT_EQUALS(remaining_cooloffs(child_of_1),2);
	return true;
}
static bool testRemCooloff_Valid_OverdueForever_0(){
	ASSERT_ZERO(make_me_rt());
	
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,0));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		wait_for_completion_of_debug_work();
		busyWait(1000);
		exit(11);
	}
	
	ASSERT_EQUALS(is_SHORT(child1),-1);
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	/*
	what will happen here
	child will run as short
	child will wait for completion
	we check that cooloffs is full
	we complete
	the child goes to overdue, we get back control
	we check that it still returns 0 (the field contains -1)
	*/
	ASSERT_EQUALS(remaining_cooloffs(child1),0);
	complete_debug_work();
	ASSERT_EQUALS(remaining_cooloffs(child1),0);
	return true;
}


static bool test_fork_stress(){
	ASSERT_ZERO(make_me_other());
	sched_yield();
	int cooloffs = 5;
	int requested_time = 50;
	sched_param_t params = {1, 2000, 5};
	sched_param_t paramSet;
	int i, j;
	// flush_scheduling_statistic();
	if(fork()==0) {
	 	ASSERT_ZERO(sched_setscheduler(getpid(), SCHED_SHORT, &params)); //now we are short
		for(i=0;i<300;i++){
			int child = fork();
			if(child==0){
  				for(j=0;j<300;j++){
  					int grandchild = fork();
  					if(grandchild==0){
  						int temp = (is_SHORT(getpid()) == 1) ^ (is_SHORT(getpid()) == 0);
  						// ASSERT_EQUALS(temp,1);
  						if(i==4 && j == 3){
  							//print_runqueue();
  						}
  						_exit(0);
  					} else {
  						 printf("grandchild %d created", grandchild);
 // 						for(int j=0; j<20000; ++j);
  						int died = wait(0);
  						 printf(", %d died\n", died);
  						ASSERT_EQUALS(died, grandchild);
  						ASSERT_EQUALS((wait(0)<=0), 1);
  					}
  				}
				_exit(0);
			} else {
				 printf("child %d created", child);
				int j;
				for(j=0; j<20000; ++j);
				int died = wait(0);
				 printf(", %d died\n", died);
				ASSERT_EQUALS(died, child);
				ASSERT_EQUALS((wait(0)<=0), 1);
			}
		}
		_exit(0);
	} else {
		while(wait(0)>0);
	}
	// print_log(getpid());

	return true;
}


static bool testGetParam_CooloffNotGoDown(){
	ASSERT_ZERO(make_me_rt());
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(500);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	// int sched_priority;
    // int requested_time;
    // int number_of_cooloffs;
	
	sched_param_t params;
	sched_getparam(child1, &params);
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	ASSERT_EQUALS(params.number_of_cooloffs,5);
	ASSERT_EQUALS(params.requested_time,100);
	
	
	
	return true;
}

static bool testSetParam_CooloffsNotChangingButReqTimeYes(){
	ASSERT_ZERO(make_me_rt());
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(500);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	// int sched_priority;
    // int requested_time;
    // int number_of_cooloffs;
	
	sched_param_t params= {1,166,1};
	sched_setparam(child1, &params);
	sched_getparam(child1, &params);
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	ASSERT_EQUALS(params.number_of_cooloffs,5);
	ASSERT_EQUALS(params.requested_time,166);
	
	
	
	return true;
}

static bool testSetParam_UsedAsSetSchedulerWith_Neg1_ShouldWork(){
	ASSERT_ZERO(make_me_rt());
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(500);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	// int sched_priority;
    // int requested_time;
    // int number_of_cooloffs;
	
	sched_param_t params= {1,166,1};
	ASSERT_ZERO(sched_setscheduler(child1,-1, &params));
	sched_getparam(child1, &params);
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	ASSERT_EQUALS(params.number_of_cooloffs,5);
	ASSERT_EQUALS(params.requested_time,166);
	
	
	
	return true;
}

static bool testSetParam_UsedAsSetSchedulerWith_OtherNegatives_ShouldFailOnPERM(){
	ASSERT_ZERO(make_me_rt());
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(500);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	// int sched_priority;
    // int requested_time;
    // int number_of_cooloffs;
	
	sched_param_t params= {1,166,1};
	ASSERT_EQUALS(sched_setscheduler(child1,-5, &params),-1);
	ASSERT_EQUALS(errno,EPERM);
	sched_getparam(child1, &params);
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	ASSERT_EQUALS(params.number_of_cooloffs,5);
	ASSERT_EQUALS(params.requested_time,100);
	
	
	
	return true;
}
static bool testSetParam_UsedAsSetSchedulerWith_EveryPolicyIncludingSHORT_ShouldFailOnPERM(){
	ASSERT_ZERO(make_me_rt());
	pid_t child1 = fork();
	if(child1==0){
		nice(1);
		ASSERT_ZERO(make_rt_child_become_short_with_params(child1,100,5));
		sched_yield(); // yields as RT, comes back to next line as SHORT
		busyWait(500);
		exit(11);
	}
	
	ASSERT_ZERO(make_me_other());
	sched_yield();
	
	// int sched_priority;
    // int requested_time;
    // int number_of_cooloffs;
	
	sched_param_t params= {1,166,1};
	int policy;
	for(policy=0;policy<=10;policy++){
		ASSERT_EQUALS(sched_setscheduler(child1,policy, &params),-1);
		ASSERT_EQUALS(errno,EPERM);
	}
	sched_getparam(child1, &params);
	ASSERT_EQUALS(remaining_cooloffs(child1),4);
	ASSERT_EQUALS(params.number_of_cooloffs,5);
	ASSERT_EQUALS(params.requested_time,100);
	
	
	
	return true;
}


int main(void) {
	RUN_TEST(testSimpleShorts_superSimple);
	// RUN_TEST(testSimpleShorts_SamePrioRunInFIFO);
	// RUN_TEST(testSimpleShorts_BestBecomesWorst_ImmediateSwitchAndRunInNewOrder);
	// RUN_TEST(testYieldingShorts_SingleBestYields_NoSwitch);
	// RUN_TEST(testYieldingShorts_OneOfBestsYields_GoesBackToEndOfHisPrio);
	// RUN_TEST(testForkingShorts_TwoProcs_ChildRunsFirst);
	// RUN_TEST(testForkingShorts_TwoProcs_ChildBecomesNicer_SwitchBackToParent);
	// RUN_TEST(testForkingShorts_MoreProcs_ChildRunsFirst_ParentRunsLastInPrio);
	// RUN_TEST(testWaitingShorts_BestWaits_ComesBackToFirst);
	// RUN_TEST(testSimpleOverdueShorts_ExecuteFIFO);
	// RUN_TEST(testForkingOverdueShorts_ParentRuns_ChildGoesToBackOfLine);
	// RUN_TEST(testWaitingOverdueShorts_GoesToBackOfLine);
	// RUN_TEST(testYieldingOverdueShorts_GoesToBackOfLine);
	
	// RUN_TEST(testIsShort_Invalid_OTHER);
	// RUN_TEST(testIsShort_Invalid_FIFO);
	// RUN_TEST(testIsShort_Invalid_RR);
	// RUN_TEST(testIsShort_Invalid_NotExistPID);
	// RUN_TEST(testIsShort_Invalid_NotLegalPID);
	// RUN_TEST(testIsShort_Valid_RegularShort);
	// RUN_TEST(testIsShort_Valid_RegularShortAfterFork);
	// RUN_TEST(testIsShort_Valid_OverdueShort);
	
	// RUN_TEST(testRemTime_Invalid_OTHER);
	// RUN_TEST(testRemTime_Invalid_FIFO);
	// RUN_TEST(testRemTime_Invalid_RR);
	// RUN_TEST(testRemTime_Invalid_NotExistPID);
	// RUN_TEST(testRemTime_Invalid_NotLegalPID);
	
	// RUN_TEST(testRemCooloff_Invalid_OTHER);
	// RUN_TEST(testRemCooloff_Invalid_FIFO);
	// RUN_TEST(testRemCooloff_Invalid_RR);
	// RUN_TEST(testRemCooloff_Invalid_NotExistPID);
	// RUN_TEST(testRemCooloff_Invalid_NotLegalPID);
	// RUN_TEST(testRemCooloff_Valid_ShortAfterFork_HalfNumber_OddOriginal);
	// RUN_TEST(testRemCooloff_Valid_ShortAfterFork_HalfNumber_EvenOriginal);
	// RUN_TEST(testRemCooloff_Valid_ShortAfterFork_HalfNumber_SingleOriginal);
	// RUN_TEST(testRemCooloff_Valid_ShortAfterFork_HalfNumber_ZeroOriginal);
	// RUN_TEST(testRemCooloff_Valid_Overdue_ShouldAlreadyBeLess);
	// RUN_TEST(testRemCooloff_Valid_OverdueAfterFork_HalfNumber);
	// RUN_TEST(testRemCooloff_Valid_OverdueForever_0);
	
	// RUN_TEST(testGetParam_CooloffNotGoDown);
	// RUN_TEST(testSetParam_CooloffsNotChangingButReqTimeYes);
	// RUN_TEST(testSetParam_UsedAsSetSchedulerWith_Neg1_ShouldWork);
	// RUN_TEST(testSetParam_UsedAsSetSchedulerWith_OtherNegatives_ShouldFailOnPERM);
	// RUN_TEST(testSetParam_UsedAsSetSchedulerWith_EveryPolicyIncludingSHORT_ShouldFailOnPERM);
	
	
	// //Valka stress test
	// RUN_TEST(test_fork_stress);
	return 0;
}

