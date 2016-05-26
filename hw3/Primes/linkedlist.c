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
			LL_free();
			return;
		}

		next->num = i;
		next->prev = current;
		next->next = NULL;

		if (pthread_mutex_init(&(next->mutex), NULL)) {
			free(next);
			LL_free();
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

	acquire(current->next);
	if (!current->next)
		release(current);
	release(current->prev);

	return current->next;
}

// Delete Node from LL and return the Node after it (safe)
Node LL_remove(Node current) {
	if (!current)
		return NULL;

	if (current->next) {
		acquire(current->next);
		current->next->prev = current->prev;
	}
	else {
		release(current->prev);
		release(current);
	}

	if (current->prev)
		current->prev->next = current->next;

	Node next = current->next;

	release(current);
//	free(current);

	return next;
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
	return pthread_mutex_lock(&(current->mutex));
}

int release(Node current) {
	if (!current)
		return EINVAL;
	return pthread_mutex_unlock(&(current->mutex));
}

