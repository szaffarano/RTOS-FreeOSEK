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

#include <os.h>
#include <debounce.h>

static unsigned long counter;
debounce_t sw4;

// callback para indicarle a librería de debounce si esta presionado el pulsador
static int pushed(void* args);

//#define printf(...)
int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);

	counter = 0;
	sw4 = debounce_add(20, pushed, NULL);

	StartOS(AppMode1);

	while (1)
		;

	return 0;
}

TASK(taskDebounce) {
	debounce_update(&sw4);

	if (sw4.change == FELL) {
		counter = 0;
	} else if (sw4.change == ROSE) {
		Board_LED_Set(0, true);
		SetRelAlarm(wakeUpTaskTurnOffLED, counter, 0);
	}

	TerminateTask();
}

TASK(taskTurnOffLED) {
	Board_LED_Set(0, false);
	TerminateTask();
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}

static int pushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

ALARMCALLBACK(counterHook) {
	counter++;
}
