#include <board.h>

#include <stdio.h>
#include <cr_section_macros.h>

#include <os.h>
#include <debounce.h>
#include <queue.h>

#define	DEBOUNCE_CYCLE		10
#define	DEBOUNCE_TIME		20
#define BLINKS				5
#define CONTROLLER_TIMEOUT	1000
static int is_sw4_pushed(void* args);

static debounce_t sw4;
static queue_t queue;

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// SW4 setup
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
	sw4 = debounce_add(DEBOUNCE_TIME / DEBOUNCE_CYCLE, is_sw4_pushed, NULL);

	// RGB Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, true);

	// RGB Verde
	Chip_GPIO_SetDir(LPC_GPIO, 2, 1, true);

	// RGB Azul
	Chip_GPIO_SetDir(LPC_GPIO, 0, 26, true);

	queue_init(&queue, 1, EventQueue, AlarmTimeoutPush, AlarmTimeoutPop, MutexQueue);

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskDebounce) {
	debounce_update(&sw4);
	static TaskStateType state;
	static int value;

	if (sw4.change == ROSE) {
		printf("Notifico que recibí el evento\n");
		queue_push(&queue, 1, 10);
	} else if (sw4.change == FELL) {
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, true);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);

		GetTaskState(taskController, &state);
		if (state == SUSPENDED) {
			queue_pop(&queue, &value, 10);
			printf("Activando tarea controller\n");
			ActivateTask(taskController);
		}
	}

	SetRelAlarm(wakeUpDebounce, DEBOUNCE_CYCLE, 0);

	TerminateTask();
}

TASK(taskBlink) {
	Board_LED_Toggle(0);
	SetRelAlarm(wakeUpBlink, 500, 0);
	TerminateTask();
}

TASK(taskController) {
	int value;

	if (queue_pop(&queue, &value, CONTROLLER_TIMEOUT) == QUEUE_TIMEOUT) {
		printf("Se fue por timeout, no pude leer\n");
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, false);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
	} else {
		printf("Evento ok\n");
	}

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
