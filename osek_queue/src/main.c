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

#define	PRODUCER_CYCLE	30
#define	CONSUMER_CYCLE	5

static queue_t queue;
static int counter;

static void queue_cb(queue_event_t event);

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	queue_init(&queue, 1, queue_cb);

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

static void queue_cb(queue_event_t event) {
	static TaskType taskWaitingPush = 0xFF;
	static TaskType taskWaitingPop = 0xFF;

	if (event == WAIT_EVENT_PUSH) {
		if (taskWaitingPush != 0xFF) {
			printf("queue: ya hay una tarea esperando un evento de push\n");
			ErrorHook();
		}
		GetTaskID(&taskWaitingPush);
		WaitEvent(eventQueue);
		taskWaitingPush = 0xFF;
		ClearEvent(eventQueue);
	} else if (event == FIRE_EVENT_PUSH) {
		if (taskWaitingPush == 0xFF) {
			printf("queue: no hay tarea esperando un evento de push\n");
			ErrorHook();
		}
		SetEvent(taskWaitingPush, eventQueue);
	} else if (event == WAIT_EVENT_POP) {
		if (taskWaitingPop != 0xFF) {
			printf("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		GetTaskID(&taskWaitingPop);
		WaitEvent(eventQueue);
		taskWaitingPop = 0xFF;
		ClearEvent(eventQueue);
	} else if (event == FIRE_EVENT_POP) {
		if (taskWaitingPop == 0xFF) {
			printf("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(taskWaitingPop, eventQueue);
	}
}
