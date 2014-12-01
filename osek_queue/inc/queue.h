/*
 * queue.h
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <os.h>

#define DEBUG_QUEUE 1

#define QUEUE_SIZE	6

typedef struct {
	unsigned int idx_push;
	unsigned int idx_pop;
	int data[QUEUE_SIZE];

	// datos para sincronización OSEK
	EventMaskType event;
	TaskType task;
	int waiting_event;
} queue_t;

/**
 * Inicializa la cola.
 */
void queue_init(queue_t* queue, EventMaskType event);

/**
 * Agrega un elemento a la cola.  En el caso de estar llena bloquea hasta que se libere un slot.
 * @TODO: ¿agregar timeout de bloqueo? ¿modificar la firma para que devuelva estado de la operación?
 */
void queue_push(queue_t *queue, int value);

/**
 * Quita un elemento de la cola.  Si la cola está vacía se bloquea hasta que haya algún slot ocupado.
 * @TODO: ¿agregar timeout de bloqueo? ¿modificar la firma para que devuelva estado de la operación?
 */
void queue_pop(queue_t *queue, int *value);

/**
 * Función de debug que hace un printf con la representación de la cola.
 */
void queue_dump(queue_t *queue);

#endif /* QUEUE_H_ */
