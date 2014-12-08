/*
 ===============================================================================
 Name        : osek_example_app.c
 Author      : Pablo Ridolfi
 Version     :
 Copyright   : (c)2014, Pablo Ridolfi, DPLab@UTN-FRBA
 Description : main definition
 ===============================================================================
 */

#include "board.h"

#include <stdio.h>
#include <cr_section_macros.h>

#include "os.h"               /* <= operating system header */

#include <queue.h>

#define	PRODUCER_CYCLE	300
#define	CONSUMER_CYCLE	500

static queue_t queue;
static int counter;

static void wait_event(void*);
static void fire_event(void*);

static TaskType taskWaiting;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	queue_init(&queue, 5, wait_event, fire_event);

	counter = 0;

	StartOS(AppMode1);

	while (1)
		;

	return 0;
}

TASK(taskToggle) {
	Board_LED_Toggle(0);
	TerminateTask();
}

TASK(taskProducer) {
	static int counter = 0;

	printf("Task Producer: pushing value %d\n", counter);
	queue_push(&queue, counter);

	SetRelAlarm(activateProducer, PRODUCER_CYCLE, 0);

	counter++;

	TerminateTask();
}

TASK(taskConsumer) {
	static int value;

	queue_pop(&queue, &value);
	printf("Task Consumer: popped value %d\n", value);

	SetRelAlarm(activateConsumer, CONSUMER_CYCLE, 0);

	TerminateTask();
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}

static void wait_event(void* d) {
	if (taskWaiting) {
		printf("queue: ERROR! ya hay una tarea esperando un evento\n");
		ErrorHook();
	}
	GetTaskID(&taskWaiting);
	WaitEvent(eventQueue);
	taskWaiting = 0;
	ClearEvent(eventQueue);
}

static void fire_event(void* d) {
	if (!taskWaiting) {
		printf("queue: ERROR! no hay tarea esperando, imposible disparar evento\n");
		ErrorHook();
	}
	SetEvent(taskWaiting, eventQueue);
}
