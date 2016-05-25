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


Node findCandidate(Node p, Node* pp2) {
	Node p2 = *pp2;
	p2 = LL_next(p);
	while (p2) {
		printf("Looking at %d (for %d)", p2->num, p->num);
		*pp2 = p2;
		if (p2->num == p->num * p->num)
			return p;

		p2 = LL_next(p2);
	}
	return NULL;
}

void deleteMultiples(Node current, int num) {
	while (current) {
		if (current->num % num == 0) {
			printf("Deleting %d\n", current->num);
			current = LL_remove(current);
		}
		else
			current = LL_next(current);
	}
}

void printLog() {
	;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Y u no give 2 arguments??\n");
		return -1;
	}

	int N = atoi(argv[1]);
	int T = atoi(argv[2]);

	if (N < 2 || T < 1) {
		printf("Invalid arguments");
		return -1;
	}

	// Init - test
	LL_getRangeFrom2(N);

	int i;
	for (i = 1; i < T; i++) {
		pid_t pid = fork();
		if (pid == 0)
			break;
	}
	printf("Here\n");
	Node p2 = NULL;
	Node p = findCandidate(LL_head(), &p2);
	while (p) {
		printf("Found %d. ", p->num);
		release(LL_remove(p2));		// release p2 + 1
		Node next = LL_next(p);
		release(p);
		deleteMultiples(next, p->num);
	}

	wait(NULL);
	printLog();
	LL_free();

	return 1;
}

