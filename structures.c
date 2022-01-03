#include "helpers.h"
#include "structures.h"

Elem *new_elem(void *data) {
	Elem *elem = malloc(sizeof(Elem));

	DIE(!elem, "[server] ERROR: Couldn't alloc new element\n");

	elem->data = data;
	elem->next = NULL;

	return elem;
}

Queue *new_queue() {
	Queue *queue = malloc(sizeof(Queue));

	DIE(!queue, "[server] ERROR: Couldn't alloc new queue\n");

	queue->head = NULL;
	queue->tail = NULL;

	return queue;
}

int queue_is_empty(Queue *q) {
	return q->head == NULL;
}

void enqueue(Queue *q, void *e) {
	Elem *elem = new_elem(e);

	if (queue_is_empty(q)) {
		q->head = elem;
		q->tail = elem;
	} else {
		q->tail->next = elem;
		q->tail = elem;
	}
}

void *dequeue(Queue *q) {
	if (queue_is_empty(q)) {
		return NULL;
	}

	Elem *head_elem = q->head;

	// advance
	q->head = q->head->next;

	if (q->head == NULL) {
		// queue became empty
		q->tail = NULL;
	}

	void *to_return = head_elem->data;

	free(head_elem);

	return to_return;
}

void delete_queue(Queue *q) {
	Elem *curr_elem = NULL;

	while ((curr_elem = dequeue(q))) {
		free(curr_elem);
	}
}