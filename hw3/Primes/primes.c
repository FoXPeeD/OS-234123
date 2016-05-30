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
#include <pthread.h>
//#endif



// Gets node p, deletes all of its multiples (stops if p^2 is missing), and returns the next p
Node handleCandidate(Node p, FILE* f, int i) {
	if (!p)
		return NULL;


	Node p2 = LL_next(p);
	int isFirstThread = 0;
	while (p2 && (isFirstThread || p2->num <= p->num * p->num)) {
//		printf("Looking at %d (for %d) by thread %d\n", p2->num, p->num, i);
		if (p2->num == p->num * p->num) {
			isFirstThread = 1;
			fprintf(f, "Prime %d (by %d).\n", p->num, i);
		}

		// Delete if needed, move on either way.
		if (p2->num % p->num == 0) {
			fprintf(f, "%d\n", p2->num);
			p2 = LL_remove(p2);
		}
		else
			p2 = LL_next(p2);
	}


	// Reached the end of the list
//	printf("Done with prime number %d\n", p->num);
	int res;
	if (p2) {
//		printf("Releasing prev ##%d (thread %d)\n", (p2->prev)? p2->prev->num : 0, i);
		res = release(p2->prev);		// TODO: What should i do with the result?
//		printf("Releasing ##%d (thread %d)\n", (p2)? p2->num : 0, i);
		res = release(p2);		// TODO: What should i do with the result?
	}

//	printf("Trying to acquire prev ##%d (thread %d)\n", (p->prev)? p->prev->num : 0, i);
	res = acquire(p->prev);		// TODO: What should i do with the result?
//	printf("Acquired prev ##%d (thread %d)\n", (p->prev)? p->prev->num : 0, i);

//	printf("Trying to acquire ##%d (thread %d)\n", (p)? p->num : 0, i);
	res = acquire(p);		// TODO: What should i do with the result?
//	printf("Acquired ##%d (thread %d)\n", (p)? p->num : 0, i);

	p = LL_next(p);
//	printf("Next prime to look at is %d\n", p->num);
	return p;
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

//	printf("All done with all of the primes! Now releasing %d and %d\n", (p->prev)? p->prev->num : 0, (p)? p->num : 0);
	release(p->prev);
	release(p);

//	LL_logAll(info.f);

//	fprintf(info.f, "Done with i %d\n", info.i);
	fclose(info.f);

	return NULL;
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

//	counter = 0;
//	max_counter = 0;
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
	pthread_t pthreads[T];
	for (j = 0; j < T - 1; j++) {
		pthread_create(&pthread, NULL, thread_do, &(arg_threads[j]));
		pthreads[j] = pthread;
	}

	f = firstf;
	i = 1;

	Node p = handleCandidate(LL_head(), f, i);

	while (p && p->num * p->num <= N){
		p = handleCandidate(p, f, i);
	}
	release(p->prev);
	release(p);

	fclose(f);

	f = fopen("primes.log", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    return 1;
	}

	LL_logAll(f);
	fclose(f);
	LL_free();

	for (j = 0; j < T - 1; j++) {
		pthread_join(pthreads[j], NULL);
	}

	free(arg_threads);


	return 0;
}

