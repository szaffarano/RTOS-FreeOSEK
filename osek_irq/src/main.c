/*
 ===============================================================================
 Name        : osek_example_app.c
 Author      : Pablo Ridolfi
 Version     :
 Copyright   : (c)2014, Pablo Ridolfi, DPLab@UTN-FRBA
 Description : main definition
 ===============================================================================
 */

#define	DEBOUNCE_TIME_MILLIS	20

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
#endif
#endif

	// inicializar interrupcion para el pin 15
	Chip_GPIOINT_Init(LPC_GPIOINT);
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));

	printf("Starting OSEK-OS in AppMode1\n");
	StartOS(AppMode1);

	while (1)
		;

	return 0;
}

void ErrorHook(void) {
	ShutdownOS(0);
}

TASK(TareaLED) {
	Board_LED_Toggle(0);
	TerminateTask();
}

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
		ActivateTask(TareaLED);
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
