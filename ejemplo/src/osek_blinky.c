/*
 ===============================================================================
 Name        : osek_example_app.c
 Author      : Pablo Ridolfi
 Version     :
 Copyright   : (c)2014, Pablo Ridolfi, DPLab@UTN-FRBA
 Description : main definition
 ===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <stdio.h>
#include <cr_section_macros.h>

#include "os.h"               /* <= operating system header */

/*
 * See the OIL configuration file in
 * etc/config.oil
 */

/* Comment this line in order to use LPC-Link based printf()
 * If uncommented, all printf() calls will be disabled
 */
//#define printf(...)
int main(void) {
#if defined (__USE_LPCOPEN)
#if !defined(NO_BOARD_LIB)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	Board_LED_Set(0, false);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 0, 26, 1);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 2, 0, 1);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 2, 1, 1);

#endif
#endif

	StartOS(AppMode1);

	/* we shouldn't return here */
	while (1)
		;

	return 0;
}

void ErrorHook(void) {
	/* kernel panic :( */
	ShutdownOS(0);
}

TASK(TaskInit) {
	/* Set 500 tick alarm for TaskPeriodic */
	SetRelAlarm(ActivateTaskBlink, 0, 500);

	SetRelAlarm(ActivateTaskGreen, 500, 0);

	/* end InitTask */
	TerminateTask();
}

TASK(TaskBlink) {
	printf("bink\n");
	Board_LED_Toggle(0);
	TerminateTask();
}

TASK(TaskGreen) {
	printf("green\n");
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, 0);
	SetRelAlarm(ActivateTaskBlue, 500, 0);
	TerminateTask();
}
TASK(TaskBlue) {
	printf("blue\n");
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, 1);
	SetRelAlarm(ActivateTaskRed, 500, 0);
	TerminateTask();
}
TASK(TaskRed) {
	printf("red\n");
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 1, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, 0);
	SetRelAlarm(ActivateTaskGreen, 500, 0);
	TerminateTask();
}
