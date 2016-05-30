/*
 * primes.c
 *
 *  Created on: May 25, 2016
 *      Author: Ben Weiss
 */

#include "linkedlist.h"

#include <time.h>

//#ifdef ECLIPSE
//#define fork() 1
//#define wait(node);
//#else
#include <unistd.h>
//#endif

int counter;
int max_counter;


// Gets node p, deletes all of its multiples (stops if p^2 is missing), and returns the next p
Node handleCandidate(Node p, FILE* f, int i) {
	if (!p)
		return NULL;

	Node p2 = LL_next(p);
	int isFirstThread = 0;
	while (p2 && (isFirstThread || p2->num <= p->num * p->num)) {
//		printf("Looking at %d (for %d)\n", p2->num, p->num);
		if (p2->num == p->num * p->num) {
			isFirstThread = 1;
			fprintf(f, "Prime %d (by %d).\n", p->num, i);
		}

		// Delete if needed, move on either way.
		if (p2->num % p->num == 0) {

//			struct timeval tv;
//			gettimeofday(&tv, NULL);
//
//			int time_in_mill = tv.tv_usec;
//
//			fprintf(f, "%d: (stored in %d at %d ms)\n", p2->num, p2, time_in_mill);
			p2 = LL_remove(p2, f);
		}
		else
			p2 = LL_next(p2);
	}


	// Reached the end of the list
	int res;
	if (p2) {
		res = release(p2->prev);		// TODO: What should i do with the result?
		res = release(p2);		// TODO: What should i do with the result?
	}

	res = acquire(p->prev);		// TODO: What should i do with the result?
	res = acquire(p);		// TODO: What should i do with the result?
	return LL_next(p);
}

struct thread_info_t {
	FILE *f;
	int i;
	int N;

};

typedef struct thread_info_t Thread_info;


void* thread_do(void* param){
	Thread_info info = *(Thread_info*)param;
	Node p = handleCandidate(LL_head(), info.f, info.i);

	while (p && p->num * p->num <= info.N)
		p = handleCandidate(p, info.f, info.i);

//	LL_logAll(info.f);

	fprintf(info.f, "Done with i %d\n", info.i);
	fclose(info.f);

	return NULL;
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	counter = 0;
	max_counter = 0;
	pthread_mutex_t total_threads_lock;
	pthread_mutex_init(&(total_threads_lock), NULL);
	int total_threads = 1;

	if (argc != 3) {
		printf("Y u no give 2 arguments??\n");
		return -1;
	}

	int N = atoi(argv[1]);
	int T = atoi(argv[2]);

	if (N < 2 || T < 1) {
		printf("Invalid arguments\n");
		return -1;
	}

	// Init
	LL_getRangeFrom2(N);

	FILE *f = fopen("thread-1.log", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    return 1;
	}

	FILE* firstf = f;
	int i;
	pid_t pid;
	Thread_info* arg_threads = (Thread_info*) malloc(sizeof(Thread_info)*(T-1));
	for (i = 2; i <= T; i++) {
//		pid = fork();
//		if (pid == 0) {
			char fname[260];
			sprintf(fname, "thread-%d.log", i);
			f = fopen(fname, "w");
//			printf("%s stored at address: %d\n", fname, *f);
//			fprintf(f, "I am %d\n", i);
			if (f == NULL)
			{
			    printf("Error opening file!\n");
			    return 1;
			}
			arg_threads[i-2].f = f;
			arg_threads[i-2].i = i;
			arg_threads[i-2].N = N;
//			break;
//		}
	}
	int j = 0;
	pthread_t pthread;
	for( j=0 ; j < T-1 ; j++){
		pthread_create(&pthread,NULL,thread_do,&(arg_threads[j]));
	}
//	fprintf(f, "the last i was %d\n", i);
//	printf("%d's head address: %d\n", i, LL_head());

	f = firstf;
	i = 1;

	Node p = handleCandidate(LL_head(), f, i);

	while (p && p->num * p->num <= N){
		p = handleCandidate(p, f, i);
	}
//	LL_logAll(f);
//
//	LL_free(f);


	fprintf(f, "Done with i %d\n", i);
	fclose(f);

//	if (!pid)		// If is a son
//	{
//		return 0;
//	}

	f = fopen("primes.log", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    return 1;
	}

	if (T==1) LL_print_locked();
	LL_logAll(f);

	LL_free(f);
	fclose(f);

	free(arg_threads);

	printf("max simultaneous locks %d\n", max_counter);

	return 0;
}

