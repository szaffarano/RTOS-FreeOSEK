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

#define	QUEUE_TIMEOUT_INCREMENT	10

/**
 * Calcula el siguiente índice en la cola circular
 */
static inline int next_idx(int idx, unsigned int size);

/**
 * Espera por el evento para continuar con la operación de push
 */
static queue_status_t queue_wait_event_push(queue_t *queue, unsigned long timeout, int next_push);

/**
 * Si hay otra tarea que está bloqueada esperando el evento para hacer push, lo dispara
 */
static void queue_fire_event_push(queue_t *queue);

/**
 * Espera por el evento para continuar con la operación de push
 */
static queue_status_t queue_wait_event_pop(queue_t *queue, unsigned long timeout);

/**
 * Si hay otra tarea que está bloqueada esperando el evento para hacer push, lo dispara
 */
static void queue_fire_event_pop(queue_t *queue);

void queue_init(queue_t* queue, unsigned int size, EventMaskType eventQueue, AlarmType alarmTimeoutPush, AlarmType alarmTimeoutPop) {
	queue->idx_pop = 0;
	queue->idx_push = 0;
	queue->size = (size + 1) < MAX_QUEUE_SIZE ? (size + 1) : MAX_QUEUE_SIZE;
	queue->blocked_by_pop = 0;
	queue->blocked_by_push = 0;

	// osek stuff
	queue->eventQueue = eventQueue;
	queue->alarmTimeoutPush = alarmTimeoutPush;
	queue->alarmTimeoutPop = alarmTimeoutPop;
	queue->taskWaitingPush = 0xFF;
	queue->taskWaitingPop = 0xFF;
	queue->taskWaitingTimeoutPush = 0xFF;
	queue->taskWaitingTimeoutPop = 0xFF;
}

queue_status_t queue_push(queue_t *queue, int value, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;

	int next_push = next_idx(queue->idx_push, queue->size);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("queue_push: No pude hacer push, cola llena!\n");
		status = queue_wait_event_push(queue, timeout, next_push);

		if (status != QUEUE_OK) {
			return status;
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

	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		q_debug("queue_pop: No pude hacer pop, cola vacia\n");
		status = queue_wait_event_pop(queue, timeout);

		if (status != QUEUE_OK) {
			return status;
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

static queue_status_t queue_wait_event_push(queue_t *queue, unsigned long timeout, int next_push) {
	queue_status_t status = QUEUE_OK;
	unsigned long elapsedTime = 0;

	queue->blocked_by_push = 1;
	q_debug("Invocando callback wait_event_push\n");

	if (queue->taskWaitingPush != 0xFF) {
		q_debug("queue_wait_event_push: ya hay una tarea esperando un evento de push\n");
		ErrorHook();
	}

	GetTaskID(&queue->taskWaitingPush);
	if (timeout == 0) {
		q_debug("queue_wait_event_push: esperando evento push con timeout infinito\n");

		WaitEvent(queue->eventQueue);
		ClearEvent(queue->eventQueue);

		q_debug("queue_wait_event_push: wait_event devolvió el control\n");
	} else {
		if (queue->taskWaitingTimeoutPush != 0xFF) {
			q_debug("queue_wait_event_pop: ya hay una tarea esperando un evento de timeout\n");
			ErrorHook();
		}
		q_debug("queue_wait_event_push: esperando evento push con timeout: %d\n", timeout);
		GetTaskID(&queue->taskWaitingTimeoutPush);
		while (1) {
			if (next_push != queue->idx_pop || elapsedTime > timeout) {
				break;
			}
			SetRelAlarm(queue->alarmTimeoutPush, QUEUE_TIMEOUT_INCREMENT, 0);
			WaitEvent(queue->eventQueue);
			ClearEvent(queue->eventQueue);
			elapsedTime += QUEUE_TIMEOUT_INCREMENT;
		}
		queue->taskWaitingTimeoutPush = 0xFF;
		if (next_push == queue->idx_pop) {
			q_debug("queue_wait_event_push: Timeout\n");
			status = QUEUE_TIMEOUT;
		} else {
			q_debug("queue_wait_event_push: no se fue por timeout\n");
		}
	}
	queue->taskWaitingPush = 0xFF;

	return status;
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

static queue_status_t queue_wait_event_pop(queue_t *queue, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;
	unsigned long elapsedTime = 0;

	queue->blocked_by_pop = 1;
	q_debug("Invocando callback wait_event_pop\n");

	if (queue->taskWaitingPop != 0xFF) {
		q_debug("queue_wait_event_pop: ya hay una tarea esperando un evento de pop\n");
		ErrorHook();
	}

	GetTaskID(&queue->taskWaitingPop);
	if (timeout == 0) {
		q_debug("queue_wait_event_pop: esperando evento pop con timeout infinito\n");

		WaitEvent(queue->eventQueue);
		ClearEvent(queue->eventQueue);

		q_debug("queue_wait_event_pop: wait_event devolvió el control\n");
	} else {
		if (queue->taskWaitingTimeoutPop != 0xFF) {
			q_debug("queue_wait_event_pop: ya hay una tarea esperando un evento de timeout\n");
			ErrorHook();
		}
		q_debug("queue_wait_event_pop: esperando evento pop con timeout: %d\n", timeout);

		GetTaskID(&queue->taskWaitingTimeoutPop);
		while (1) {
			if (queue->idx_push != queue->idx_pop || elapsedTime > timeout) {
				break;
			}
			SetRelAlarm(queue->alarmTimeoutPop, QUEUE_TIMEOUT_INCREMENT, 0);
			WaitEvent(queue->eventQueue);
			ClearEvent(queue->eventQueue);
			elapsedTime += QUEUE_TIMEOUT_INCREMENT;
		}
		queue->taskWaitingTimeoutPop = 0xFF;

		if (queue->idx_push == queue->idx_pop) {
			q_debug("queue_wait_event_pop: Timeout\n");
			status = QUEUE_TIMEOUT;
		} else {
			q_debug("queue_wait_event_pop: no se fue por timeout\n");
		}
	}
	queue->taskWaitingPop = 0xFF;

	return status;
}

static void queue_fire_event_pop(queue_t *queue) {
	if (queue->blocked_by_pop) {
		q_debug("Invocando callback fire_event_pop\n");
		if (queue->taskWaitingPop == 0xFF) {
			q_debug("queue_fire_event_pop: no hay ninguna tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(queue->taskWaitingPop, queue->eventQueue);
		queue->blocked_by_pop = 0;
	}
}
