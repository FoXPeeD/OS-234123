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

	Node first = (Node)malloc(sizeof(Node*));
	if (!first)
		return;
	head = first;
	first->num = 2;
	first->prev = NULL;
	first->next = NULL;
	if(pthread_mutex_init( &(first->mutex), NULL ))
		free(first);
		return;
	Node prev = first;
	for (int i = 3; i < y; i++) {
		Node curr = (Node)malloc(sizeof(Node*));
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
Node LL_head() {
	//TODO: Acquire lock for *head
	return *head;
}


// Return the Node after the specified one (safe)
Node LL_next(Node current) {
	if (!current)
		return NULL;

	acquire(current->next);
	release(current->prev);
	return current->next;
}


// Delete Node from LL and return the Node after it (safe)
Node LL_remove(Node current) {

}


// Destructor
void LL_destory() {

}


void acquire(Node current) {
	if (!current)
		return;

	// TODO: Acquire the lock
}

void release(Node current) {
	if (!current)
		return;
	// TODO: Release the lock
}



