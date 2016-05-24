/*
 * linkedlist.h
 *
 *  Created on: May 24, 2016
 *      Author: Ben Weiss
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include<stdio.h>
#include<stdlib.h>

struct node_t {
	int num;
	struct node_t *prev;
	struct node_t *next;
	// lock lock;
};

typedef struct node_t* node;

node head;

// Return LL filled with ints from 2 until y (inclusive)
void LL_getRangeFrom2(int y);

// Return the head (safe)
node LL_head();

// Return the node after the specified one (safe)
node LL_next(node current);

// Delete node from LL and return the node after it (safe)
node LL_remove(node current);

// Destructor
void LL_destory();





#endif /* LINKEDLIST_H_ */
