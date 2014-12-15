/*
 * queue.h
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <OsekApi.h>

#define DEBUG_QUEUE 1

#define MAX_QUEUE_SIZE	128

typedef enum {
	WAIT_EVENT_PUSH, FIRE_EVENT_PUSH, WAIT_EVENT_POP, FIRE_EVENT_POP
} queue_event_t;

typedef enum {
	QUEUE_OK, QUEUE_TIMEOUT, QUEUE_UNKNOWN_ERROR
} queue_status_t;

typedef void (*queue_event_cb_t)(queue_event_t);

typedef struct {
	int data[MAX_QUEUE_SIZE];

	unsigned int size;
	unsigned int idx_push;
	unsigned int idx_pop;

	int blocked_by_push;
	int blocked_by_pop;

	// osek stuff
	EventMaskType eventQueue;
	AlarmType alarmQueue;
	TaskType taskWaitingPush;
	TaskType taskWaitingPop;
	TaskType taskWaitingTimeout;
} queue_t;

/**
 * Inicializa la cola.  Recibe el tamaño de la cola.
 */
void queue_init(queue_t* queue, unsigned int size, EventMaskType eventQueue, AlarmType alarmQueue);

/**
 * Agrega un elemento a la cola.  En el caso de estar llena bloquea hasta que se libere un slot.
 * @TODO: ¿agregar timeout de bloqueo? ¿modificar la firma para que devuelva estado de la operación?
 */
queue_status_t queue_push(queue_t *queue, int value, unsigned long timeout);

/**
 * Quita un elemento de la cola.  Si la cola está vacía se bloquea hasta que haya algún slot ocupado.
 * @TODO: ¿agregar timeout de bloqueo? ¿modificar la firma para que devuelva estado de la operación?
 */
queue_status_t queue_pop(queue_t *queue, int *value, unsigned long timeout);

/**
 * Función de debug que hace un printf con la representación de la cola.
 */
void queue_dump(queue_t *queue);

#endif /* QUEUE_H_ */
