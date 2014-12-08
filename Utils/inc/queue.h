/*
 * queue.h
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#define DEBUG_QUEUE 1

#define QUEUE_SIZE	6

typedef void(*queue_event_cb)(void*);

typedef struct {
	int data[QUEUE_SIZE];

	unsigned int idx_push;
	unsigned int idx_pop;
	queue_event_cb wait_event_cb;
	queue_event_cb fire_event_cb;
	int waiting_event;
} queue_t;


/**
 * Inicializa la cola.
 */
void queue_init(queue_t* queue, queue_event_cb wait_cb, queue_event_cb fire_cb);

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
