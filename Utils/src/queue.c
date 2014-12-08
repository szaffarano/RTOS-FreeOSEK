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
 * Espera por el evento para continuar con la operación (push / pop)
 */
static void queue_wait_event(queue_t *queue);

/**
 * Si hay otra tarea que está bloqueada esperando el evento, lo dispara
 */
static void queue_fire_event(queue_t *queue);

void queue_init(queue_t* queue, unsigned int size, queue_event_cb wait_cb, queue_event_cb fire_cb) {
	queue->idx_pop = 0;
	queue->idx_push = 0;
	queue->size = (size+1)< MAX_QUEUE_SIZE ? (size+1) : MAX_QUEUE_SIZE;
	queue->wait_event_cb = wait_cb;
	queue->fire_event_cb = fire_cb;
	queue->waiting_event = 0;
}

void queue_push(queue_t *queue, int value) {
	int next_push = next_idx(queue->idx_push, queue->size);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("queue_push: No pude hacer push, cola llena!\n");
		queue_wait_event(queue);
	}

	queue->idx_push = next_push;
	queue->data[next_push] = value;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event(queue);
}

void queue_pop(queue_t *queue, int *value) {
	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		q_debug("queue_pop: No pude hacer pop, cola vacia\n");
		queue_wait_event(queue);
	}

	int i = next_idx(queue->idx_pop, queue->size);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event(queue);
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

static void queue_wait_event(queue_t *queue) {
	queue->waiting_event = 1;
	q_debug("Invocando callback wait_event\n");
	queue->wait_event_cb(queue);
	q_debug("Callback wait_event devolvió el control\n");
}

static void queue_fire_event(queue_t *queue) {
	if (queue->waiting_event) {
		q_debug("Invocando callback fire_event\n");
		queue->fire_event_cb(queue);
		queue->waiting_event = 0;
	}
}
