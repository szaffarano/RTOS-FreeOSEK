/*
 * queue.c
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#include <queue.h>
#include <stdio.h>

static inline int next_idx(int idx);

void queue_init(queue_t* queue, EventMaskType event) {
	int i = 0;
	for (; i < QUEUE_SIZE; i++) {
		queue->data[i] = -1;
	}
	queue->idx_pop = 0;
	queue->idx_push = 0;

	// OSEK
	queue->event = event;
	queue->waiting_push = 0;
	queue->waiting_pop = 0;
}

void queue_push(queue_t *queue, int value) {
	static TaskType t;
	GetTaskID(&t);
	printf("[%d] queue_push\n", t);

	int next_push = next_idx(queue->idx_push);

	if (next_push == queue->idx_pop) {
		// cola llena, espero evento de cola vacía
		printf("[%d] queue_push: No pude hacer push, cola llena!\n", t);

		GetTaskID(&(queue->task));
		queue->waiting_pop = 1;

		printf("[%d] queue_push: Esperando evento %d para tarea %d\n", t, queue->event, queue->task);

		WaitEvent(queue->event);

		printf("[%d] queue_push: Se recibió evento %d para tarea %d\n", t, queue->event, queue->task);

		ClearEvent(queue->event);
	}

	queue->idx_push = next_push;
	queue->data[next_push] = value;

	// si hay una tarea esperando un evento, notifico
	if (queue->waiting_push) {
		printf("[%d] queue_push: Disparando evento %d para tarea %d\n",t, queue->event, queue->task);

		SetEvent(queue->task, queue->event);
		queue->waiting_push = 0;
	}
}

void queue_pop(queue_t *queue, int *value) {
	static TaskType t;
	GetTaskID(&t);
	printf("[%d] queue_pop\n", t);

	if (queue->idx_push == queue->idx_pop) {
		// cola vacía, espero evento
		printf("[%d] queue_pop: No pude hacer pop, cola vacia\n", t);

		GetTaskID(&(queue->task));
		queue->waiting_push = 1;
		printf("[%d] queue_pop: Esperando evento %d para tarea %d\n", t, queue->event, queue->task);

		WaitEvent(queue->event);
		printf("[%d] queue_pop: Se recibió el evento %d para tarea %d\n", t, queue->event, queue->task);
		ClearEvent(queue->event);
	}
	int i = next_idx(queue->idx_pop);
	*value = queue->data[i];
	queue->data[i] = -1;
	queue->idx_pop = i;

	// si hay una tarea esperando un evento, notifico
	if (queue->waiting_pop) {
		printf("[%d] queue_popo: Disparando evento %d para tarea %d\n", t, queue->event, queue->task);
		SetEvent(queue->task, queue->event);
		queue->waiting_pop = 0;
	}
}

void queue_dump(queue_t *queue) {
	int i = 0;
	printf("[");
	for (; i < QUEUE_SIZE; i++) {
		printf("%3d%s", queue->data[i], (i + 1) == QUEUE_SIZE ? "" : ", ");
	}
	printf("]\n");
}

static inline int next_idx(int idx) {
	return (idx + 1) % QUEUE_SIZE;
}

#if DEBUG_QUEUE
int main() {
	queue_t q;
	int i, v;

	queue_init(&q);

	printf("--- comienzo\n");
	queue_dump(&q);

	printf("--- bucle push\n");
	for (i = 0; i < 5; i++) {
		queue_push(&q, i);
		queue_dump(&q);
	}
	printf("--- bucle pop\n");
	for (i = 0; i < 5; i++) {
		queue_pop(&q, &v);
		queue_dump(&q);
	}
	return 0;
}
#endif
