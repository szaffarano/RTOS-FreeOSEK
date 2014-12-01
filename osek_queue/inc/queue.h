/*
 * queue.h
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <os.h>

#define QUEUE_SIZE	6

typedef struct {
	unsigned int idx_push;
	unsigned int idx_pop;
	int data[QUEUE_SIZE];

	// datos para sincronización OSEK
	EventMaskType event;
	TaskType task;
	int waiting_push;
	int waiting_pop;
} queue_t;

void queue_init(queue_t* queue, EventMaskType event);
void queue_push(queue_t *queue, int value);
void queue_pop(queue_t *queue, int *value);
void queue_dump(queue_t *queue);

#endif /* QUEUE_H_ */
