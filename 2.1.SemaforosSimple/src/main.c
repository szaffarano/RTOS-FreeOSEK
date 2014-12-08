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
static void wait_event(void*);
static void fire_event(void*);
static int is_sw4_pushed(void* args);

static debounce_t sw4;
static TaskType taskWaiting;
static unsigned int blinkCounter;
static queue_t queue;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// SW4 setup
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
	sw4 = debounce_add(DEBOUNCE_TIME / DEBOUNCE_CYCLE, is_sw4_pushed, NULL);

	queue_init(&queue, 10, wait_event, fire_event);

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
		printf(
				"queue: ERROR! no hay tarea esperando, imposible disparar evento\n");
		ErrorHook();
	}

	SetEvent(taskWaiting, eventQueue);
}
