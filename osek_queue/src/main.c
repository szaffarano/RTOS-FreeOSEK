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

#define	PRODUCER_CYCLE		500
#define PRODUCER_TIMEOUT	1000
#define	CONSUMER_CYCLE		300
#define CONSUMER_TIMEOUT	100

static unsigned long ticks;
static queue_t queue;
static int counter;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	queue_init(&queue, 1, EventQueue);

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
	} else {
		printf("Task Producer: Funciono el timeout\n");
	}

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
	} else {
		printf("Task Consumer: Funciono el timeout\n");

	}
	printf("Task Consumer: popped value %d\n", value);

	SetRelAlarm(activateConsumer, CONSUMER_CYCLE, 0);

	TerminateTask();
}

TASK(TaskTimeout) {
	SetEvent(queue.taskWaitingPush, EventQueue);
	TerminateTask();
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}

ALARMCALLBACK(counterHook) {
	ticks++;
}

unsigned long SystemTicks() {
	return ticks;
}
