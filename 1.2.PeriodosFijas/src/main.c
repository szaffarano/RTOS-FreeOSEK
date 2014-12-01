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

//#define printf(...)
int main(void) {
	SystemCoreClockUpdate();

	Board_Init();

	Board_LED_Set(0, false);

	StartOS(AppMode1);

	while (1)
		;

	return 0;
}

TASK(taskBlink) {
	static int step = 0;
	Board_LED_Set(0, true);
	SetRelAlarm(wakeUpTaskTurnOffLED, ++step * 100, 0);
	step = step == 10 ? 0 : step;
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
