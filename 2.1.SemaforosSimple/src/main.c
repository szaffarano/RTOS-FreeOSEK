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

	queue_init(&queue, 1, EventQueue, AlarmTimeoutPush, AlarmTimeoutPop);

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskDebounce) {
	debounce_update(&sw4);

	if (sw4.change == ROSE) {
		queue_push(&queue, 1, 0);
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
	queue_pop(&queue, &value, 0);

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

/* lógica de manejo de timeout de colas */
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
