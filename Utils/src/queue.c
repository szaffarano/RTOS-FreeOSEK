/*
 * queue.c
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#include <queue.h>
#include <stdio.h>
#include <stdarg.h>

/* ############################## [macros] ##############################*/
#if DEBUG_QUEUE
#define q_debug(format, ...) printf (format, ## __VA_ARGS__)
#else
#define q_debug(format, ...)
#endif

#define	QUEUE_TIMEOUT_INCREMENT	10
#define	NULL_TASK				0xFF

/* ############################## [encabezados de funciones privadas] ##############################*/
static inline int next_idx(int idx, unsigned int size);
static queue_status_t queue_wait_event_push(queue_t *queue, unsigned long timeout, int next_push);
static void queue_fire_event_push(queue_t *queue);
static queue_status_t queue_wait_event_pop(queue_t *queue, unsigned long timeout);
static void queue_fire_event_pop(queue_t *queue);

/* ############################## [implementaciones] ##############################*/

void queue_init(queue_t* queue, unsigned int size, EventMaskType eventQueue, AlarmType alarmTimeoutPush,
		AlarmType alarmTimeoutPop) {
	queue->idx_pop = 0;
	queue->idx_push = 0;
	queue->size = (size + 1) < MAX_QUEUE_SIZE ? (size + 1) : MAX_QUEUE_SIZE;
	queue->blocked_by_pop = 0;
	queue->blocked_by_push = 0;

	// osek stuff
	queue->event_queue = eventQueue;
	queue->alarm_timeout_push = alarmTimeoutPush;
	queue->alarm_timeout_pop = alarmTimeoutPop;

	queue->task_waiting_push = NULL_TASK;
	queue->task_waiting_pop = NULL_TASK;
	queue->task_waiting_timeout_push = NULL_TASK;
	queue->task_waiting_timeout_pop = NULL_TASK;
}

queue_status_t queue_push(queue_t *queue, int value, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;

	int next_push = next_idx(queue->idx_push, queue->size);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		q_debug("queue_push: Cola llena\n");

		// @TODO no enviar parámetro "next_push", recalcularlo cuando haga falta.
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
		q_debug("queue_pop: Cola vacía\n");
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
	q_debug("queue_wait_event_push: Invocando wait_event_push\n");

	/* Validar que no haya otra tarea que está bloqueada esperando hacer push */
	if (queue->task_waiting_push != NULL_TASK) {
		q_debug("queue_wait_event_push: ya hay una tarea que hizo waiting_push\n");
		ErrorHook();
	}

	/* setear tarea que está haciendo push para referencias cruzadas */
	GetTaskID(&queue->task_waiting_push);

	/* Timeout = 0 -> espera infinita */
	if (timeout == 0) {
		q_debug("queue_wait_event_push: Esperando push [timeout infinito]\n");

		WaitEvent(queue->event_queue);
		ClearEvent(queue->event_queue);

		q_debug("queue_wait_event_push: wait_event devolvió el control\n");
	} else {
		/* validar que no haya otra tarea que está bloqueada esperando timeout */
		if (queue->task_waiting_timeout_push != NULL_TASK) {
			q_debug("queue_wait_event_pop: ya hay una tarea que hizo waiting_timeout_push\n");
			ErrorHook();
		}
		q_debug("queue_wait_event_push: Esperando push [timeout: %d]\n", timeout);
		GetTaskID(&queue->task_waiting_timeout_push);
		while (1) {
			if (next_push != queue->idx_pop || elapsedTime > timeout) {
				break;
			}
			SetRelAlarm(queue->alarm_timeout_push, QUEUE_TIMEOUT_INCREMENT, 0);
			WaitEvent(queue->event_queue);
			ClearEvent(queue->event_queue);
			elapsedTime += QUEUE_TIMEOUT_INCREMENT;
		}
		queue->task_waiting_timeout_push = NULL_TASK;
		if (next_push == queue->idx_pop) {
			q_debug("queue_wait_event_push: Timeout\n");
			status = QUEUE_TIMEOUT;
		} else {
			q_debug("queue_wait_event_push: Se devolvió el control antes de irse por timeout\n");
		}
	}
	queue->task_waiting_push = NULL_TASK;

	return status;
}

static void queue_fire_event_push(queue_t *queue) {
	if (queue->blocked_by_push) {
		q_debug("queue_fire_event_push\n");
		if (queue->task_waiting_push == NULL_TASK) {
			q_debug("queue_fire_event_push: no hay tarea esperando un evento de push\n");
			ErrorHook();
		}
		SetEvent(queue->task_waiting_push, queue->event_queue);
		queue->blocked_by_push = 0;
	}
}

static queue_status_t queue_wait_event_pop(queue_t *queue, unsigned long timeout) {
	queue_status_t status = QUEUE_OK;
	unsigned long elapsedTime = 0;

	queue->blocked_by_pop = 1;
	q_debug("queue_wait_event_pop: Invocando wait_event_pop\n");

	/* Validar que no haya otra tarea que está bloqueada esperando hacer pop */
	if (queue->task_waiting_pop != NULL_TASK) {
		q_debug("queue_wait_event_pop: ya hay una tarea que hizo waiting_pop\n");
		ErrorHook();
	}

	/* setear tarea que está haciendo pop para referencias cruzadas */
	GetTaskID(&queue->task_waiting_pop);

	/* Timeout = 0 -> espera infinita */
	if (timeout == 0) {
		q_debug("queue_wait_event_pop: esperando evento [timeout infinito]\n");

		WaitEvent(queue->event_queue);
		ClearEvent(queue->event_queue);

		q_debug("queue_wait_event_pop: wait_event devolvió el control\n");
	} else {
		/* validar que no haya otra tarea que está bloqueada esperando timeout */
		if (queue->task_waiting_timeout_pop != NULL_TASK) {
			q_debug("queue_wait_event_pop: ya hay una tarea que hizo waiting_timeout_pop\n");
			ErrorHook();
		}
		q_debug("queue_wait_event_pop: esperando evento [timeout: %d]\n", timeout);

		GetTaskID(&queue->task_waiting_timeout_pop);
		while (1) {
			if (queue->idx_push != queue->idx_pop || elapsedTime > timeout) {
				break;
			}
			SetRelAlarm(queue->alarm_timeout_pop, QUEUE_TIMEOUT_INCREMENT, 0);
			WaitEvent(queue->event_queue);
			ClearEvent(queue->event_queue);
			elapsedTime += QUEUE_TIMEOUT_INCREMENT;
		}
		queue->task_waiting_timeout_pop = NULL_TASK;

		if (queue->idx_push == queue->idx_pop) {
			q_debug("queue_wait_event_pop: Timeout\n");
			status = QUEUE_TIMEOUT;
		} else {
			q_debug("queue_wait_event_pop: Se devolvió el control antes de irse por timeout\n");
		}
	}
	queue->task_waiting_pop = NULL_TASK;

	return status;
}

static void queue_fire_event_pop(queue_t *queue) {
	if (queue->blocked_by_pop) {
		q_debug("queue_fire_event_pop\n");
		if (queue->task_waiting_pop == NULL_TASK) {
			q_debug("queue_fire_event_pop: no hay ninguna tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(queue->task_waiting_pop, queue->event_queue);
		queue->blocked_by_pop = 0;
	}
}
