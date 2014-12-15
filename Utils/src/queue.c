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

/**
 * Calcula el siguiente índice en la cola circular
 */
static inline int next_idx(int idx, unsigned int size);

/**
 * Espera por el evento para continuar con la operación de push
 */
static void queue_wait_event_push(queue_t *queue);

/**
 * Si hay otra tarea que está bloqueada esperando el evento para hacer push, lo dispara
 */
static void queue_fire_event_push(queue_t *queue);

/**
 * Espera por el evento para continuar con la operación de push
 */
static void queue_wait_event_pop(queue_t *queue);

/**
 * Si hay otra tarea que está bloqueada esperando el evento para hacer push, lo dispara
 */
static void queue_fire_event_pop(queue_t *queue);

void queue_init(queue_t* queue, unsigned int size, EventMaskType event) {
	queue->idx_pop = 0;
	queue->idx_push = 0;
	queue->size = (size + 1) < MAX_QUEUE_SIZE ? (size + 1) : MAX_QUEUE_SIZE;
	queue->blocked_by_pop = 0;
	queue->blocked_by_push = 0;

	// osek stuff
	queue->event = event;
	queue->taskWaitingPush = 0xFF;
	queue->taskWaitingPop = 0xFF;
}

void queue_push(queue_t *queue, int value) {
	int next_push = next_idx(queue->idx_push, queue->size);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("queue_push: No pude hacer push, cola llena!\n");
		queue_wait_event_push(queue);
	}

	queue->idx_push = next_push;
	queue->data[next_push] = value;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event_pop(queue);
}

void queue_pop(queue_t *queue, int *value) {
	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		q_debug("queue_pop: No pude hacer pop, cola vacia\n");
		queue_wait_event_pop(queue);
	}

	int i = next_idx(queue->idx_pop, queue->size);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event_push(queue);
}

void queue_dump(queue_t *queue) {
	int i = 0;
	q_debug("[");
	for (; i < queue->size; i++) {
		q_debug("%3d%s", queue->data[i], (i + 1) == queue->size ? "" : ", ");
	}q_debug("]\n");
}

static inline int next_idx(int idx, unsigned int size) {
	return (idx + 1) % size;
}

static void queue_wait_event_push(queue_t *queue) {
	queue->blocked_by_push = 1;
	q_debug("Invocando callback wait_event_push\n");

	if (queue->taskWaitingPush != 0xFF) {
		q_debug("queue: ya hay una tarea esperando un evento de push\n");
		ErrorHook();
	}

	GetTaskID(&queue->taskWaitingPush);
	WaitEvent(queue->event);
	queue->taskWaitingPush = 0xFF;
	ClearEvent(queue->event);

	q_debug("Callback wait_event devolvió el control\n");
}

static void queue_fire_event_push(queue_t *queue) {
	if (queue->blocked_by_push) {
		q_debug("Invocando callback fire_event_push\n");
		if (queue->taskWaitingPush == 0xFF) {
			q_debug("queue: no hay tarea esperando un evento de push\n");
			ErrorHook();
		}
		SetEvent(queue->taskWaitingPush, queue->event);
		queue->blocked_by_push = 0;
	}
}

static void queue_wait_event_pop(queue_t *queue) {
	queue->blocked_by_pop = 1;
	q_debug("Invocando callback wait_event_pop\n");

	if (queue->taskWaitingPop != 0xFF) {
		q_debug("queue: ya hay una tarea esperando un evento de pop\n");
		ErrorHook();
	}
	GetTaskID(&queue->taskWaitingPop);
	WaitEvent(queue->event);
	queue->taskWaitingPop = 0xFF;
	ClearEvent(queue->event);

	q_debug("Callback wait_event devolvió el control\n");
}

static void queue_fire_event_pop(queue_t *queue) {
	if (queue->blocked_by_pop) {
		q_debug("Invocando callback fire_event_pop\n");
		if (queue->taskWaitingPop == 0xFF) {
			q_debug("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(queue->taskWaitingPop, queue->event);
		queue->blocked_by_pop = 0;
	}
}
