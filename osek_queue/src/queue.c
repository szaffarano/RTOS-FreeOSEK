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
static inline int next_idx(int idx);

/**
 * Espera por el evento para continuar con la operación (push / pop)
 */
static void queue_wait_event(queue_t *queue);

/**
 * Si hay otra tarea que está bloqueada esperando el evento, lo dispara
 */
static void queue_fire_event(queue_t *queue);

void queue_init(queue_t* queue, EventMaskType event) {
	int i = 0;
	for (; i < QUEUE_SIZE; i++) {
		queue->data[i] = -1;
	}
	queue->idx_pop = 0;
	queue->idx_push = 0;

	// OSEK stuf
	queue->event = event;
	queue->waiting_event = 0;
}

void queue_push(queue_t *queue, int value) {
	int next_push = next_idx(queue->idx_push);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("[%u] queue_push: No pude hacer push, cola llena!\n", t);
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
		q_debug("[%u] queue_pop: No pude hacer pop, cola vacia\n", t);
		queue_wait_event(queue);
	}

	int i = next_idx(queue->idx_pop);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event(queue);
}

void queue_dump(queue_t *queue) {
	int i = 0;
	q_debug("[");
	for (; i < QUEUE_SIZE; i++) {
		q_debug("%3d%s", queue->data[i], (i + 1) == QUEUE_SIZE ? "" : ", ");
	}q_debug("]\n");
}

static inline int next_idx(int idx) {
	return (idx + 1) % QUEUE_SIZE;
}

static void queue_wait_event(queue_t *queue) {
	/* obtener id de tarea que desencadena el bloqueo */
	GetTaskID(&(queue->task));

	/* @TODO: usar queue->task como flag? */
	queue->waiting_event = 1;

	q_debug("[%u] Esperando evento %d para tarea %d\n", queue->task, queue->event);
	WaitEvent(queue->event);
	q_debug("[%u] Se recibió evento %d para tarea %d\n", queue->task, queue->event);
	ClearEvent(queue->event);
}

static void queue_fire_event(queue_t *queue) {
	/* sólo para debug... */
	TaskType current_task;

	/* @TODO: usar queue->task como flag? */
	if (queue->waiting_event) {
		GetTaskID(&current_task);
		q_debug("[%u] Disparando evento %d para tarea %d\n", current_task, queue->event, queue->task);
		SetEvent(queue->task, queue->event);
		queue->waiting_event = 0;
	}
}
