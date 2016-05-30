/*
 * linkedlist.h
 *
 *  Created on: May 24, 2016
 *      Author: Ben Weiss
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

struct node_t {
	int num;
	struct node_t *prev;
	struct node_t *next;
	pthread_mutex_t mutex;
	int is_locked;
};

typedef struct node_t* Node;

Node head;

// Return LL filled with ints from 2 until y (inclusive)
void LL_getRangeFrom2(int y);

// Return the head (safe)
Node LL_head();

// Return the node after the specified one (safe)
Node LL_next(Node current);

// Delete node from LL and return the node after it (safe)
Node LL_remove(Node current, FILE* f);

// Destructor
void LL_free(FILE* f);

int acquire(Node current);
int release(Node current);

void LL_print_locked(); // TODO: delete

#endif /* LINKEDLIST_H_ */
