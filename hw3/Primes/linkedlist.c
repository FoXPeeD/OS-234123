/*
 * linkedlist.c
 *
 *  Created on: May 24, 2016
 *      Author: Ben Weiss
 */

#include "linkedlist.h"

//extern int counter;
//extern int max_counter;

// Returns LL filled with ints from 2 until y (inclusive)
void LL_getRangeFrom2(int y) {
	head = NULL;
	if (y < 2)
		return;

	Node first = (Node) malloc(sizeof(struct node_t));
	if (!first)
		return;

	head = first;
	first->num = 2;
	first->prev = NULL;
	first->next = NULL;

	if (pthread_mutex_init(&(first->mutex), NULL)) {
		free(first);
		return;
	}

	Node current = first;
	int i;
	for (i = 3; i <= y; i++) {
		Node next = (Node) malloc(sizeof(struct node_t));
		if (!next) {
			LL_free(); // COMMENTED OUT ONLY TEMPORARILY
			return;
		}

		next->num = i;
		next->prev = current;
		next->next = NULL;

		if (pthread_mutex_init(&(next->mutex), NULL)) {
			free(next);
			LL_free(); // SAME
			return;
		}
		current->next = next;
		next->prev = current;
		current = next;
	}
}

// Return the head (safe)
Node LL_head() {
	acquire(head);
	return head;
}

// Return the Node after the specified one (safe)
Node LL_next(Node current) {
	if (!current)
		return NULL;


//	printf("LL_NEXT: Trying to acquire NEXT ##%d ##\n", (current->next)? current->next->num : 0);
	acquire(current->next);
	if (!current->next) {
//		printf("LL_NEXT: Releasing CURRENT ##%d ##\n", (current)? current->num : 0);
		release(current);
	}
//	printf("LL_NEXT: Releasing PREV ##%d ##\n", (current->prev)? current->prev->num : 0);
	release(current->prev);

	return current->next;
}

// Delete Node from LL and return the Node after it (safe)
Node LL_remove(Node current) {
	if (!current)
		return NULL;

	if (current->prev)
		current->prev->next = current->next;

	if (current->next) {
		acquire(current->next);
		current->next->prev = current->prev;
	}
	else {
		release(current->prev);
		release(current);
	}


	Node next = current->next;

	release(current);

//	fprintf(f, "Just deleted %d, stored in %d\n", current->num, current);
//	fprintf(f, "%d\n", current->num);
//	if (current->prev && current->next)
//		fprintf(f, "Now it's %d -> %d\n", current->prev->num, current->prev->next->num);

	free(current);

	return next;
}

void LL_logAll(FILE* f) {		// Unsafe - only do when no other threads are running.
	Node p = head;
	while (p) {
		fprintf(f, "%d\n", p->num);
		p = LL_next(p);
	}
}

// Destructor
void LL_free() {
	acquire(head);
	Node current = head;
	while (current)
		current = LL_remove(current);
}

int acquire(Node current) {
	if (!current)
		return EINVAL;
	int res = pthread_mutex_lock(&(current->mutex));
//	counter++;
//	max_counter = (counter > max_counter) ? counter : max_counter;
	current->is_locked = 1;
	return res;
}

int release(Node current) {
	if (!current)
		return EINVAL;
//	counter--;
//	current->is_locked = 0;
	return pthread_mutex_unlock(&(current->mutex));
}

////for use only with 1 thread
//void LL_print_locked(){
//	Node current = head;
//	printf("currently locked: ");
//	while (current){
//		if(current->is_locked){
//			printf("%d ",current->num);
//		}
//		current = current->next;
//	}
//	printf("\n");
//}
