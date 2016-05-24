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
	head = first;
	first->num = 2;
	first->prev = NULL;
	first->next = NULL;
	if(pthread_mutex_init( &(first->mutex), NULL ))
		free(first);
		return;
	node prev = first;
	for (int i = 3; i < y; i++) {
		node curr = (node)malloc(sizeof(node*));
		if (!curr)
			LL_destory();
			return;
		curr->num = i;
		curr->prev = NULL;
		curr->next = NULL;
		if(pthread_mutex_init( &(first->mutex), NULL )){
			free(curr);
			LL_destory();
			return;
		}
		prev->next = curr;
		curr->prev = prev;
		prev = curr;
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


