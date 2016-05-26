/*
 * primes.c
 *
 *  Created on: May 25, 2016
 *      Author: Ben Weiss
 */

#include "linkedlist.h"


#ifdef ECLIPSE
#define fork() 1
#define wait(node);
#else
#include <unistd.h>
#endif


// Gets node p, deletes all of its multiples (stops if p^2 is missing), and returns the next p
Node handleCandidate(Node p) {
	if (!p)
		return NULL;

	printf("About to handle %d", p->num);

	Node p2 = LL_next(p);
	int isFirstThread = 0;
	while (p2 && (isFirstThread || p2->num <= p->num * p->num)) {
		printf("Looking at %d (for %d)\n", p2->num, p->num);

		if (p2->num == p->num * p->num)
			isFirstThread = 1;

		// Delete if needed, move on either way.
		if (p2->num % p->num == 0) {
			printf("Deleting %d\n", p2->num);
			p2 = LL_remove(p2);
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


void printLog() {
	;
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

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

	int i;
	for (i = 1; i < T; i++) {
		pid_t pid = fork();
		if (pid == 0)
			break;
	}

	Node p = handleCandidate(LL_head());

//	printf("Next prime to deal with is %d.\n", p? p->num : 0);
	while (p) {
		printf("Found prime %d.\n", p->num);
		Node p = handleCandidate(p);
	}

	wait(NULL);
	printLog();
	LL_free();

	return 1;
}

