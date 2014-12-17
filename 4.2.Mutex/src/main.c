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

#define	PULSE_ALERT_TIME		400
#define	CONTROLLER_BLINK_TIME	200

static int is_sw4_pushed(void* args);

static debounce_t sw4;
static queue_t queue;
static queue_t mutex;
static unsigned long ticks;

#define printf(...)
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
	queue_init(&mutex, 1, EventQueue, AlarmTimeoutPush, AlarmTimeoutPop, MutexQueue);

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskDebounce) {
	debounce_update(&sw4);
	static int enlapsed_time;

	if (sw4.change == ROSE) {
		enlapsed_time = ticks - enlapsed_time;
		printf("taskDebounce: Enviando nuevo tiempo [%d]\n", enlapsed_time);
		if (queue_push(&queue, enlapsed_time, 0) == QUEUE_OK) {
			printf("taskDebounce: Nuevo tiempo enviado\n");
		} else {
			printf("taskDebounce: No se envió nuevo tiempo, se fue por timeout\n");
		}
	} else if (sw4.change == FELL) {
		enlapsed_time = ticks;
	}

	SetRelAlarm(wakeUpDebounce, DEBOUNCE_CYCLE, 0);

	TerminateTask();
}

TASK(taskBlink) {
	Board_LED_Toggle(0);
	SetRelAlarm(wakeUpBlink, 200, 0);
	TerminateTask();
}

/* blinkea el rgb rojo de la base board cada (CONTROLLER_BLINK_TIME / 2) ms salvo que hayan tomado el mutex */
TASK(taskController) {
	static int value;

	// tomo el mutex para poder laburar sin problemas
	printf("taskController: push[mutex]\n");
	queue_push(&mutex, value, 0);
	printf("taskController: push[mutex] taked\n");

	// prendo el led y espero
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);

	SetRelAlarm(wakeUpController, CONTROLLER_BLINK_TIME - 50, 0);

	printf("taskController: waiting EventLED [1]\n");
	WaitEvent(EventLED);
	ClearEvent(EventLED);
	printf("taskController: EventLED [1] cleared\n");

	// apago el led y espero el resto de tiempo
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);

	SetRelAlarm(wakeUpController, CONTROLLER_BLINK_TIME, 0);
	printf("taskController: waiting EventLED [2]\n");
	WaitEvent(EventLED);
	ClearEvent(EventLED);
	printf("taskController: EventLED [2] cleared\n");

	// libero el mutex para que pueda usarlo otra tarea
	printf("taskController: pop[mutex]\n");
	queue_pop(&mutex, &value, 0);
	printf("taskController: pop[mutex] released\n");

	// programo lanzamiento de la tarea nuevamente
	printf("taskController: setting activateController\n");
	SetRelAlarm(activateController, 0, 0);

	TerminateTask();
}

TASK(taskPulse) {
	static int value;
	static int flag;

	// recivo el valor
	printf("taskPulse: pop[queue]\n");
	queue_pop(&queue, &value, 0);
	printf("taskPulse: pop[queue]: %d\n", value);

	// tomo el recurso para poder usar el led
	printf("taskPulse: push[mutex]\n");
	queue_push(&mutex, flag, 0);
	printf("taskPulse: push[mutex] taked\n");

	// dejo apagado el led 1 segundo como guardabanda
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, true);
	SetRelAlarm(wakeUpPulse, PULSE_ALERT_TIME, 0);

	printf("taskPulse: waiting EventLED[1]\n");
	WaitEvent(EventPulse);
	ClearEvent(EventPulse);
	printf("taskPulse: EventLED[1] cleared\n");

	// enciendo el led
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, false);

	// duermo por el tiempo que recibí
	SetRelAlarm(wakeUpPulse, value, 0);
	printf("taskPulse: waiting EventLED[2]\n");
	WaitEvent(EventPulse);
	ClearEvent(EventPulse);
	printf("taskPulse: EventLED[2] cleared\n");

	// apago el led
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);

	// vuelvo a dejar apagado el led 1 segundo como guardabanda
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, true);
	SetRelAlarm(wakeUpPulse, PULSE_ALERT_TIME, 0);

	printf("taskPulse: waiting EventLED[3]\n");
	WaitEvent(EventPulse);
	ClearEvent(EventPulse);
	printf("taskPulse: EventLED[3] cleared\n");

	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, false);

	// luego de usar el led, devuelvo el recurso
	printf("taskPulse: pop[mutex]\n");
	queue_pop(&mutex, &flag, 0);
	printf("taskPulse: pop[released]\n");

	printf("taskPulse: esperando un poco mas\n");
	SetRelAlarm(activatePulse, 0, 0);

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

/* contador de systicks */
ALARMCALLBACK(counterHook) {
	ticks++;
}
