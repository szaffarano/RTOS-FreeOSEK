#include <board.h>

#include <stdio.h>
#include <cr_section_macros.h>

#include <os.h>
#include <debounce.h>

#define	DEBOUNCE_CYCLE	5
#define	DEBOUNCE_TIME		20

static unsigned long ticks;
static unsigned long duty_cycle_millis;
debounce_t sw4;

// callback para indicarle a librería de debounce si esta presionado el pulsador
static int is_sw4_pushed(void* args);

int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);

	// interval = DEBOUNCE TIME / POLLING CYCLE
	// ej, la tarea invoca debounce_add cada 5 ms y
	// tomo como que se estabilizó a los 20 ms, entonces:
	// interval = 20 / 5 = 4
	sw4 = debounce_add(DEBOUNCE_TIME / DEBOUNCE_CYCLE, is_sw4_pushed, NULL);

	ticks = 0;
	duty_cycle_millis = 200; // valor arbitrario

	StartOS(AppMode1);

	while (1) {
	}

	return 0;
}

TASK(taskDebounce) {
	static unsigned long start_time;

	debounce_update(&sw4);

	if (sw4.change == FELL) {
		start_time = ticks;
	} else if (sw4.change == ROSE) {
		start_time = ticks - start_time;
		GetResource(mutex);
		duty_cycle_millis = start_time > 1000 ? 999 : start_time;
		ReleaseResource(mutex);
	}

	SetRelAlarm(wakeUpDebounce, DEBOUNCE_CYCLE, 0);

	TerminateTask();
}

TASK(taskBlink) {
	if (Board_LED_Test(0)) {
		Board_LED_Set(0, 0);
		GetResource(mutex);
		SetRelAlarm(wakeUpBlink, 1000 - duty_cycle_millis, 0);
		ReleaseResource(mutex);
	} else {
		Board_LED_Set(0, 1);
		GetResource(mutex);
		SetRelAlarm(wakeUpBlink, duty_cycle_millis, 0);
		ReleaseResource(mutex);
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

ALARMCALLBACK(counterHook) {
	ticks++;
}
