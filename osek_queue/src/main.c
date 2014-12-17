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

#define	PRODUCER_CYCLE		100
#define PRODUCER_TIMEOUT	1000
#define	CONSUMER_CYCLE		300
#define CONSUMER_TIMEOUT	1000

static queue_t queue;
static int counter;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	queue_init(&queue, 1, EventQueue, AlarmTimeoutPush, AlarmTimeoutPop, MutexQueue);

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

	printf("Task Producer: haciendo push [%d] con timeout %d\n", counter, PRODUCER_TIMEOUT);
	if (queue_push(&queue, counter, PRODUCER_TIMEOUT) == QUEUE_TIMEOUT) {
		printf("Task Producer: Se fue por timeout, invoco a push con timeout infinito\n");
		queue_push(&queue, counter, 0);
	}
	printf("Task Producer: push [%d] realizado\n", counter);

	SetRelAlarm(activateProducer, PRODUCER_CYCLE, 0);

	counter++;

	TerminateTask();
}

TASK(taskConsumer) {
	static int value;

	printf("Task Consumer: haciendo pop con timeout %d\n", CONSUMER_TIMEOUT);
	if (queue_pop(&queue, &value, CONSUMER_TIMEOUT) == QUEUE_TIMEOUT) {
		printf("Task Consumer: Se fue por timeout, invoco a pop con timeout infinito\n");
		queue_pop(&queue, &value, 0);
	}
	printf("Task Consumer: pop [%d]\n", value);

	SetRelAlarm(activateConsumer, CONSUMER_CYCLE, 0);

	TerminateTask();
}

TASK(TaskTimeoutPush) {
	if (queue.task_waiting_timeout_push != 0xFF) {
		SetEvent(queue.task_waiting_timeout_push, EventQueue);
	}
	TerminateTask();
}

TASK(TaskTimeoutPop) {
	if (queue.task_waiting_timeout_pop != 0xFF) {
		SetEvent(queue.task_waiting_timeout_pop, EventQueue);
	}
	TerminateTask();
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}
