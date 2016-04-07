#include "blocker.h"
#include "test_utilities.h"
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

bool block_program_test() {
  	const char* name1 = "/root/hw1_tests/prog1";
	const char* name2 = "/root/hw1_tests/prog2";
  	const char* name3 = "";
  	int len = strlen(name1);
  	ASSERT_TEST(block_program(NULL, 5) == -1);
  	ASSERT_TEST(errno == EINVAL);
  	ASSERT_TEST(block_program(name3, 0) == -1);
    ASSERT_TEST(errno == EINVAL);
 	ASSERT_TEST(block_program(name1, 0) == -1);
    ASSERT_TEST(errno == EINVAL);
  	ASSERT_TEST(block_program(name2, len) == 0);
  	ASSERT_TEST(block_program(name1, len) == 0);
    ASSERT_TEST(block_program(name2, len) == 1);
    ASSERT_TEST(block_program(name1, len) == 1);
    char *argva[] = {"/root/hw1_tests/prog1", 0};
    char *argvb[] = {"/root/hw1_tests/prog2", 0};
    
    //fails
	execv(name1, argva);
	execv(name2, argvb);

    unblock_program (name1, len);
    unblock_program (name2, len);
	return true;
}



bool unblock_test() {
	const char* name1 = "/root/hw1_tests/prog1";
	const char* name2 = "/root/hw1_tests/prog2";
	const char* name3 = "/root/hw1_tests/prog3";
	const char* name4 = "/root/hw1_tests/prog4";
	int name_len = strlen(name1);
	block_program(name1, name_len);
	block_program(name2, name_len);
	block_program(name3, name_len);
	block_program(name4, name_len);
	ASSERT_TEST(unblock_program(NULL, 3) == -1);
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(unblock_program("d", 0) == -1);
	ASSERT_TEST(errno == EINVAL);
	//wasn't blocked
	ASSERT_TEST(unblock_program("d", 1) == 1);
	char *argva[] = {"/root/hw1_tests/prog1", 0};
    char *argvb[] = {"/root/hw1_tests/prog2", 0};
    
	ASSERT_TEST(unblock_program(name1, name_len) == 0);
	int status;
	if(fork() == 0) {	//should succeed
		execv(name1, argva);
	}
	//new addition
	wait(&status);
    assert(WEXITSTATUS(status) == 0); 

	//already unblocked, should fail
	ASSERT_TEST(unblock_program(name1, name_len) == 1);
	//unblocking the rest
	ASSERT_TEST(unblock_program(name2, name_len) == 0);
	ASSERT_TEST(unblock_program(name3, name_len) == 0);
	ASSERT_TEST(unblock_program(name4, name_len) == 0);

	return true;
}


bool get_blocked_count_test() {
	ASSERT_TEST(get_blocked_count() == 0);
	const char* name1 = "prog1";
	const char* name2 = "prog2";
    const char* name3 = "prog3";
	block_program(name1, 5);
	ASSERT_TEST(get_blocked_count() == 1);
	block_program(name1, 5);
	ASSERT_TEST(get_blocked_count() == 1);
	block_program(name2, 5);
	ASSERT_TEST(get_blocked_count() == 2);
	unblock_program(name1, 5);
	ASSERT_TEST(get_blocked_count() == 1);
    block_program(NULL, 5);
    ASSERT_TEST(get_blocked_count() == 1);
    block_program(name3, 0);
    ASSERT_TEST(get_blocked_count() == 1);
	unblock_program(name2, 5);
	ASSERT_TEST(get_blocked_count() == 0);
	return true;
}


bool is_program_blocked_test() {
	const char* name1 = "/root/hw1_tests/prog1";
	const char* name2 = "/root/hw1_tests/prog2";
	const char* name3 = "/root/hw1_tests/prog3";
	const char* name4 = "/root/hw1_tests/prog4";
	int len = strlen(name1);

	//checking errors
	ASSERT_TEST(is_program_blocked(NULL, len));
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(is_program_blocked("da", 0));
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(is_program_blocked(NULL, 0));
	ASSERT_TEST(errno == EINVAL);

	block_program(name1, len);
	block_program(name2, len);

	ASSERT_TEST(is_program_blocked(name1, len));
	ASSERT_TEST(is_program_blocked(name2, len));
	ASSERT_TEST(is_program_blocked(name3, len) == 0);
	ASSERT_TEST(is_program_blocked(name4, len) == 0);
	ASSERT_TEST(is_program_blocked("adar", 4) == 0);

	block_program(name3, len);
	block_program(name4, len);

	ASSERT_TEST(is_program_blocked(name3, len));
	ASSERT_TEST(is_program_blocked(name4, len));

	unblock_program(name3, len);
	ASSERT_TEST(is_program_blocked(name1, len));
	ASSERT_TEST(is_program_blocked(name2, len));
	ASSERT_TEST(is_program_blocked(name3, len) == 0);
	ASSERT_TEST(is_program_blocked(name4, len));

	unblock_program(name4, len);
	ASSERT_TEST(is_program_blocked(name1, len));
	ASSERT_TEST(is_program_blocked(name2, len));
	ASSERT_TEST(is_program_blocked(name3, len) == 0);
	ASSERT_TEST(is_program_blocked(name4, len) == 0);

	//errors again because why not
	ASSERT_TEST(is_program_blocked(NULL, len));
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(is_program_blocked("da", 0));
	ASSERT_TEST(errno == EINVAL);
	ASSERT_TEST(is_program_blocked(NULL, 0));
	ASSERT_TEST(errno == EINVAL);

	unblock_program(name1, len);
	ASSERT_TEST(is_program_blocked(name1, len) == 0);
	ASSERT_TEST(is_program_blocked(name2, len));
	ASSERT_TEST(is_program_blocked(name3, len) == 0);
	ASSERT_TEST(is_program_blocked(name4, len) == 0);

	unblock_program(name2, len);
	ASSERT_TEST(is_program_blocked(name1, len) == 0);
	ASSERT_TEST(is_program_blocked(name2, len)== 0);
	ASSERT_TEST(is_program_blocked(name3, len) == 0);
	ASSERT_TEST(is_program_blocked(name4, len) == 0);

	return true;
}

void init_log(char log[][256], int log_len) {
	int i;
	for(i = 0; i < log_len; i++) {
		log[i][0]= 't';
		log[i][1]= 'e';
		log[i][2]= 's';
		log[i][3]= 't';
		log[i][4] = '\0';
	}
}

bool log_test() {
	const char* name1 = "/root/hw1_tests/prog1";
	const char* name2 = "/root/hw1_tests/prog2";
	const char* name3 = "/root/hw1_tests/prog3";
	const char* name4 = "/root/hw1_tests/prog4";
	const char* test_str = "test";
	int len = strlen(name1);
	block_program(name1, len);
	block_program(name2, len);
	char *argva[] = {"/root/hw1_tests/prog1", 0};
    char *argvb[] = {"/root/hw1_tests/prog2", 0};
    char *argvc[] = {"/root/hw1_tests/prog3", 0};
    char *argvd[] = {"/root/hw1_tests/prog4", 0};


    // should all fail
    execv(name1, argva);
    execv(name2, argvb);
    //this fails just because the programs don't exist
    execv(name3, argvc);
    execv(name4, argvd);
    //a few more times (logged fails)
    execv(name1, argva);
    execv(name2, argvb);
    execv(name2, argva);
    execv(name1, argvb);
    //final result: 1, 2, 1, 2, 2, 1

  	int log_len = 6;
  	char log[log_len][256];
   	
  	init_log(log, log_len);

  	ASSERT_TEST(get_forbidden_tries(-2, log, 3) == -1);
  	ASSERT_TEST(errno == ESRCH);
  	ASSERT_TEST(get_forbidden_tries(getpid(), log, 0) == -1);
  	ASSERT_TEST(errno == EINVAL);

	ASSERT_TEST(get_forbidden_tries(getpid(), log, 1) == 1);
  	for(int i = 0; i < log_len; i++) {
  		switch(i) {
  			case 0 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			default:
  				ASSERT_TEST(strcmp(log[i], test_str) == 0);
  		}
  	}

  	init_log(log, log_len);
	ASSERT_TEST(get_forbidden_tries(getpid(), log, 40) == log_len);
	
  	for(int i = 0; i < log_len; i++) {
  		switch(i) {
  			case 0 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			case 1 :
  				ASSERT_TEST(strcmp(log[i], name2) == 0);
  				break;
  			case 2 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			case 3 :
  				ASSERT_TEST(strcmp(log[i], name2) == 0);
  				break;
  			case 4 :
  				ASSERT_TEST(strcmp(log[i], name2) == 0);
  				break;
  			case 5 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			default:
  				ASSERT_TEST(strcmp(log[i], test_str) == 0);
  		}
  	}

  	init_log(log, log_len);
 	ASSERT_TEST(get_forbidden_tries(getpid(), log, 3) == 3);
  	for(int i = 0; i < log_len; i++) {
  		switch(i) {
  			case 0 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			case 1 :
  				ASSERT_TEST(strcmp(log[i], name2) == 0);
  				break;
  			case 2 :
  				ASSERT_TEST(strcmp(log[i], name1) == 0);
  				break;
  			default:
  				ASSERT_TEST(strcmp(log[i], test_str) == 0);
  		}
  	}
  	int status;
  	if(fork() == 0) {
  		init_log(log, log_len);
  		ASSERT_TEST(get_forbidden_tries(getpid(), log, log_len) == 0);
  		for(int i = 0; i < log_len; i++) {
			ASSERT_TEST(strcmp(log[i], test_str) == 0);
  		}
  		_exit(EXIT_SUCCESS);
  	}
  	wait(&status);
    assert(WEXITSTATUS(status) == EXIT_SUCCESS); 
	unblock_program(name1, len);
	unblock_program(name2, len);
  	
    return true;
}

int main() {
	RUN_TEST(unblock_test);
	RUN_TEST(get_blocked_count_test);
	RUN_TEST(is_program_blocked_test);
	RUN_TEST(log_test);
	RUN_TEST(block_program_test);
	return 0;
}

