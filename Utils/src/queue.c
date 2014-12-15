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

void queue_init(queue_t* queue, unsigned int size, EventMaskType eventQueue, AlarmType alarmQueue) {
	queue->idx_pop = 0;
	queue->idx_push = 0;
	queue->size = (size + 1) < MAX_QUEUE_SIZE ? (size + 1) : MAX_QUEUE_SIZE;
	queue->blocked_by_pop = 0;
	queue->blocked_by_push = 0;

	// osek stuff
	queue->eventQueue = eventQueue;
	queue->alarmQueue = alarmQueue;
	queue->taskWaitingPush = 0xFF;
	queue->taskWaitingPop = 0xFF;
}

queue_status_t queue_push(queue_t *queue, int value, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;
	unsigned long elapsedTime = 0;

	int next_push = next_idx(queue->idx_push, queue->size);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("queue_push: No pude hacer push, cola llena!\n");
		if (timeout == 0) {
			q_debug("queue_push: esperando evento push con timeout infinito\n");
			queue_wait_event_push(queue);
		} else {
			q_debug("queue_push: esperando evento push con timeout: %d\n", timeout);
			GetTaskID(&queue->taskWaitingTimeout);
			while (1) {
				if (next_push != queue->idx_pop || elapsedTime > timeout) {
					break;
				}
				SetRelAlarm(queue->alarmQueue, 1, 0);
				WaitEvent(queue->eventQueue);
				ClearEvent(queue->eventQueue);
				elapsedTime++;
			}
			queue->taskWaitingPush = 0xFF;
			if (next_push == queue->idx_pop) {
				q_debug("queue_push: No se hace push, se fue por timeout\n");
				return QUEUE_TIMEOUT;
			}
		}
	}

	queue->idx_push = next_push;
	queue->data[next_push] = value;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event_pop(queue);

	return status;
}

queue_status_t queue_pop(queue_t *queue, int *value, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;
	unsigned long elapsedTime = 0;

	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		q_debug("queue_pop: No pude hacer pop, cola vacia\n");
		if (timeout == 0) {
			q_debug("queue_pop: esperando evento pop con timeout infinito\n");
			queue_wait_event_pop(queue);
		} else {
			q_debug("queue_pop: esperando evento pop con timeout: %d\n", timeout);
			GetTaskID(&queue->taskWaitingTimeout);
			while (1) {
				if (queue->idx_push != queue->idx_pop || elapsedTime > timeout) {
					break;
				}
				SetRelAlarm(queue->alarmQueue, 1, 0);
				WaitEvent(queue->eventQueue);
				ClearEvent(queue->eventQueue);
				elapsedTime++;
			}
			queue->taskWaitingTimeout = 0xFF;
			if (queue->idx_push == queue->idx_pop) {
				q_debug("queue_pop: No se hace pop, se fue por timeout\n");
				return QUEUE_TIMEOUT;
			}
		}
	}

	int i = next_idx(queue->idx_pop, queue->size);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	queue_fire_event_push(queue);

	return status;
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
	WaitEvent(queue->eventQueue);
	queue->taskWaitingPush = 0xFF;
	ClearEvent(queue->eventQueue);

	q_debug("Callback wait_event devolvió el control\n");
}

static void queue_fire_event_push(queue_t *queue) {
	if (queue->blocked_by_push) {
		q_debug("Invocando callback fire_event_push\n");
		if (queue->taskWaitingPush == 0xFF) {
			q_debug("queue: no hay tarea esperando un evento de push\n");
			ErrorHook();
		}
		SetEvent(queue->taskWaitingPush, queue->eventQueue);
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
	WaitEvent(queue->eventQueue);
	queue->taskWaitingPop = 0xFF;
	ClearEvent(queue->eventQueue);

	q_debug("Callback wait_event devolvió el control\n");
}

static void queue_fire_event_pop(queue_t *queue) {
	if (queue->blocked_by_pop) {
		q_debug("Invocando callback fire_event_pop\n");
		if (queue->taskWaitingPop == 0xFF) {
			q_debug("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(queue->taskWaitingPop, queue->eventQueue);
		queue->blocked_by_pop = 0;
	}
}
