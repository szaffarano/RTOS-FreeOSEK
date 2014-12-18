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

#define	DEBOUNCE_TIME_MILLIS	20

static queue_t queue;
static unsigned long ticks;

#define printf(...)
int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// joystick, configurar IRQ de P0, 15
	Chip_GPIOINT_Init(LPC_GPIOINT);
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
	// por defecto todos los pines son input, no hace falta setear la dirección

	// LED Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, 1);

	// RGB Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, true);

	// RGB Verde
	Chip_GPIO_SetDir(LPC_GPIO, 2, 1, true);

	// RGB Azul
	Chip_GPIO_SetDir(LPC_GPIO, 0, 26, true);

	// habilitar interrupciones
	NVIC_EnableIRQ(EINT3_IRQn);

	queue_init(&queue, 1, EventQueue, AlarmTimeoutPush, AlarmTimeoutPop, MutexQueue);

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskBlink) {
	Board_LED_Toggle(0);
	SetRelAlarm(wakeUpBlink, 200, 0);
	TerminateTask();
}

TASK(taskLED) {
	int value;

	while (1) {
		queue_pop(&queue, &value, 0);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
		SetRelAlarm(WakeUpLED, 1000, 0);
		WaitEvent(EventLED);
		ClearEvent(EventLED);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
	}
}

/* tareas de debounce */
TASK(TareaFalling) {
	// si sigue presionado, activo interrupción rising, sino vuelvo a fallings
	if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 15)) {
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
	} else {
		Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
	}

	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, 1 << 15);

	TerminateTask();
}


TASK(TareaRising) {
	// si no está presionado, ok, caso contrario vuelvo a rising
	if (Chip_GPIO_GetPinState(LPC_GPIO, 0, 15)) {
		Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));

		// aviso que pulsaron boton
		queue_push(&queue, 1, 0);
	} else {
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
	}

	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, 1 << 15);

	TerminateTask();
}

ISR(ISRBoton) {
	// SW3 P2, 10
	// JOY DOWN P0, 15
	int fallingState = Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, GPIOINT_PORT0);
	int risingState = Chip_GPIOINT_GetStatusRising(LPC_GPIOINT, GPIOINT_PORT0);

	if (fallingState & (1 << 15)) {
		// limpiar interrupción falling
		Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, fallingState & ~(1 << 15));

		SetRelAlarm(AlarmaFalling, DEBOUNCE_TIME_MILLIS, 0);
	} else if (risingState & (1 << 15)) {
		// limpiar interrupcion rising
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, risingState & ~(1 << 15));

		SetRelAlarm(AlarmaRising, DEBOUNCE_TIME_MILLIS, 0);
	}
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

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}
