#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include "helpers.h"

typedef struct elem {
	void *data;

	// linked list
	struct elem *next;
} Elem;

Elem *new_elem(void *data);

typedef struct queue {
	Elem *head;
	Elem *tail;
} Queue;

Queue *new_queue();
int queue_is_empty(Queue *q);
void enqueue(Queue *q, void *e);
void *dequeue(Queue *q);
void delete_queue(Queue *q);

#endif