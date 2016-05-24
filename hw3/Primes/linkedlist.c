/*
 * linkedlist.c
 *
 *  Created on: May 24, 2016
 *      Author: Ben Weiss
 */

#include "linkedlist.h"

// Returns LL filled with ints from 2 until y (inclusive)
void LL_getRangeFrom2(int y) {
	head = NULL;
	if (y < 2)
		return;
	node first = (node)malloc(sizeof(node*));
	if (!first)
		return;
	first->num = 2;
	first->prev = NULL;
	first->next = NULL;
	//TODO: first->lock = init_lock();

	for (int i = 2; i < y; i++) {
		first->num = 2;
		first->prev = NULL;
		first->next = NULL;
	}
}


// Return the head (safe)
node LL_head() {

}


// Return the node after the specified one (safe)
node LL_next(node current) {

}


// Delete node from LL and return the node after it (safe)
node LL_remove(node current) {

}


// Destructor
void LL_destory() {

}


