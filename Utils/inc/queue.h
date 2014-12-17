/*
 * queue.h
 *
 *  Created on: 30/11/2014
 *      Author: sebas
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <OsekApi.h>

#define DEBUG_QUEUE 0

#define MAX_QUEUE_SIZE	128

typedef enum {
	QUEUE_OK, QUEUE_TIMEOUT, QUEUE_UNKNOWN_ERROR
} queue_status_t;

/*!
 * Estructura de datos que representa una cola que almacena enteros.
 *
 * Para utilizarla es necesario proveer un evento de sincronización y dos alarmas, las
 * cuales son usadas para manejar los timeouts de lectura y escritura.
 *
 * Las alarmas deberán activar a todas las tareas que usen o puedan usar la cola.
 *
 * El evento deberá estar declarado en todas las tareas que usen o puedan usar la cola.
 *
 * Las variables task_* son estado interno, no se deben modificar por el usuario de la cola.
 */
typedef struct {
	int data[MAX_QUEUE_SIZE];

	unsigned int size;
	unsigned int idx_push;
	unsigned int idx_pop;

	int blocked_by_push;
	int blocked_by_pop;

	// osek stuff
	EventMaskType event_queue;
	AlarmType alarm_timeout_push;
	AlarmType alarm_timeout_pop;

	// queue state
	TaskType task_waiting_push;
	TaskType task_waiting_pop;
	TaskType task_waiting_timeout_push;
	TaskType task_waiting_timeout_pop;
	ResourceType mutex;
} queue_t;

/*!
 * Inicializa la cola.
 *
 * @param[in, out] 	queue 				La cola a inicializar.
 * @param[in]		size				El tamaño de la cola.  Como máximo puede ser MAX_QUEUE_SIZE
 * @param[in]		eventQueue			Es un evento utilizado para sincronizar las tareas que usan la cola.
 * 										Todas las tareas deberán declararlo.
 * @param[in]		alarmTimeoutPush	Alarma usada para manejar el timeout al hacer un push.  Debe estar declarada
 * 										para activar a una tarea (manejada por el usuario de la cola) que dispara el
 * 										evento de sincronización.
 * @param[in]		alarmTimeoutPop		Alarma usada para manejar el timeout al hacer un pop.  Debe estar declarada
 * 										para activar a una tarea (manejada por el usuario de la cola) que dispara el
 * 										evento de sincronización.
 */
void queue_init(queue_t* queue, unsigned int size, EventMaskType eventQueue,
		AlarmType alarmTimeoutPush, AlarmType alarmTimeoutPop, ResourceType mutex);

/*!
 * Agrega un elemento a la cola.  En el caso de estar llena bloquea hasta que se libere un slot.
 * @param[in, out]	queue		La cola en la que se hará la operación.
 * @param[in]		value		El valor a insertar en la cola.  Si la cola está llena se bloquea según parmámetro timeout.
 * @param[in]		timeout		Si el valor del timeout es cero, la cola se bloquea indefinidamente en caso de estar llena,
 * 								sino se bloqueará por el tiempo de timeout expresado en milisegundos.
 * @return						Devuelve el estado de la operación, @see queue_status_t
 */
queue_status_t queue_push(queue_t *queue, int value, unsigned long timeout);

/**
 * Quita un elemento de la cola.  Si la cola está vacía se bloquea hasta que haya algún slot ocupado.
 * @param[in, out]	queue		La cola en la que se hará la operación.
 * @param[in]		value		Puntero en donde se guardará el valor leido.  Si la cola está vacía se bloquea según parmámetro timeout.
 * @param[in]		timeout		Si el valor del timeout es cero, la cola se bloquea indefinidamente en caso de estar vacía,
 * 								sino se bloqueará por el tiempo de timeout expresado en milisegundos.
 * @return						Devuelve el estado de la operación, @see queue_status_t
 */
queue_status_t queue_pop(queue_t *queue, int *value, unsigned long timeout);

/**
 * Función de debug que hace un printf con la representación de la cola.
 */
void queue_dump(queue_t *queue);

#endif /* QUEUE_H_ */
