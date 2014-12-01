/*
 * queue.c
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#include <queue.h>
#include <stdio.h>
#include <stdarg.h>

#if DEBUG_QUEUE
#define q_debug(format, ...) printf (format, ## __VA_ARGS__)
#else
#define q_debug(format, ...)
#endif

static inline int next_idx(int idx);

void queue_init(queue_t* queue, EventMaskType event) {
	int i = 0;
	for (; i < QUEUE_SIZE; i++) {
		queue->data[i] = -1;
	}
	queue->idx_pop = 0;
	queue->idx_push = 0;

	// OSEK stuf
	queue->event = event;
	queue->waiting_push = 0;
	queue->waiting_pop = 0;
}

void queue_push(queue_t *queue, int value) {
	static TaskType t;
	GetTaskID(&t);
	q_debug("[%u] queue_push\n", t);

	int next_push = next_idx(queue->idx_push);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("[%u] queue_push: No pude hacer push, cola llena!\n", t);

		GetTaskID(&(queue->task));
		queue->waiting_pop = 1;

		q_debug("[%u] queue_push: Esperando evento %d para tarea %d\n", t, queue->event, queue->task);

		WaitEvent(queue->event);

		q_debug("[%u] queue_push: Se recibió evento %d para tarea %d\n", t, queue->event, queue->task);

		ClearEvent(queue->event);
	}

	queue->idx_push = next_push;
	queue->data[next_push] = value;

	// si hay una tarea esperando un evento, notifico
	if (queue->waiting_push) {
		q_debug("[%u] queue_push: Disparando evento %d para tarea %d\n", t, queue->event, queue->task);

		SetEvent(queue->task, queue->event);
		queue->waiting_push = 0;
	}
}

void queue_pop(queue_t *queue, int *value) {
	static TaskType t;
	GetTaskID(&t);
	q_debug("[%u] queue_pop\n", t);

	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		q_debug("[%u] queue_pop: No pude hacer pop, cola vacia\n", t);

		GetTaskID(&(queue->task));
		queue->waiting_push = 1;
		q_debug("[%u] queue_pop: Esperando evento %d para tarea %d\n", t, queue->event, queue->task);

		WaitEvent(queue->event);
		q_debug("[%u] queue_pop: Se recibió el evento %d para tarea %d\n", t, queue->event, queue->task);
		ClearEvent(queue->event);
	}
	int i = next_idx(queue->idx_pop);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	if (queue->waiting_pop) {
		q_debug("[%u] queue_popo: Disparando evento %d para tarea %d\n", t, queue->event, queue->task);
		SetEvent(queue->task, queue->event);
		queue->waiting_pop = 0;
	}
}

void queue_dump(queue_t *queue) {
	int i = 0;
	q_debug("[");
	for (; i < QUEUE_SIZE; i++) {
		q_debug("%3d%s", queue->data[i], (i + 1) == QUEUE_SIZE ? "" : ", ");
	} q_debug("]\n");
}

static inline int next_idx(int idx) {
	return (idx + 1) % QUEUE_SIZE;
}
