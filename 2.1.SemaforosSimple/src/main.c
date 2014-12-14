#include <board.h>

#include <stdio.h>
#include <cr_section_macros.h>

#include <os.h>
#include <debounce.h>
#include <queue.h>

#define	DEBOUNCE_CYCLE		10
#define	DEBOUNCE_TIME		20
#define BLINKS				5

// callbacks
static void queue_cb(queue_event_t event);
static int is_sw4_pushed(void* args);

static debounce_t sw4;
static unsigned int blinkCounter;
static queue_t queue;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// SW4 setup
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
	sw4 = debounce_add(DEBOUNCE_TIME / DEBOUNCE_CYCLE, is_sw4_pushed, NULL);

	queue_init(&queue, 10, queue_cb);

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskDebounce) {
	debounce_update(&sw4);

	if (sw4.change == ROSE) {
		queue_push(&queue, 1);
	}

	SetRelAlarm(wakeUpDebounce, DEBOUNCE_CYCLE, 0);

	TerminateTask();
}

TASK(taskBlink) {
	GetResource(mutex);
	if (--blinkCounter > 0) {
		Board_LED_Toggle(0);
		SetRelAlarm(wakeUpBlink, 100, 0);
	} else {
		Board_LED_Set(0, 0);
	}
	ReleaseResource(mutex);
	TerminateTask();
}

TASK(taskConsumer) {
	int value;
	queue_pop(&queue, &value);

	GetResource(mutex);
	blinkCounter += BLINKS;
	if (blinkCounter == BLINKS) {
		SetRelAlarm(wakeUpBlink, 100, 0);
	}
	ReleaseResource(mutex);
	SetRelAlarm(wakeUpConsumer, 500, 0);

	TerminateTask();
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}

static int is_sw4_pushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

static void queue_cb(queue_event_t event) {
	static TaskType taskWaitingPush;
	static TaskType taskWaitingPop;

	if (event == WAIT_EVENT_PUSH) {
		if (taskWaitingPush) {
			printf("queue: ya hay una tarea esperando un evento de push\n");
			ErrorHook();
		}
		GetTaskID(&taskWaitingPush);
		WaitEvent(eventQueue);
		taskWaitingPush = 0;
		ClearEvent(eventQueue);
	} else if (event == FIRE_EVENT_PUSH) {
		if (!taskWaitingPush) {
			printf("queue: no hay tarea esperando un eventu de push\n");
			ErrorHook();
		}
		SetEvent(taskWaitingPush, eventQueue);
	} else if (event == WAIT_EVENT_POP) {
		if (taskWaitingPop) {
			printf("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		GetTaskID(&taskWaitingPop);
		WaitEvent(eventQueue);
		taskWaitingPop = 0;
		ClearEvent(eventQueue);
	} else if (event == FIRE_EVENT_POP) {
		if (!taskWaitingPop) {
			printf("queue: ya hay una tarea esperando un evento de pop\n");
			ErrorHook();
		}
		SetEvent(taskWaitingPop, eventQueue);
	}
}

