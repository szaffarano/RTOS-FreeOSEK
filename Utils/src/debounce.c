/*
 * debounce.c
 *
 *  Created on: 12/11/2014
 *      Author: sebas
 */

#include <debounce.h>

typedef enum _state_t {
	WAITING_PUSH, NOISE_PUSH, PUSHING, NOISE_RELEASE
} state_t;

typedef struct _debounce_data_t {
	int id;
	is_pressed_cb is_pressed;
	void* args;
	state_t state;
	int ticks;
	int max_ticks;
} debounce_data_t;

static debounce_data_t debounce_data[5];
static int idx;

debounce_t debounce_add(int interval, is_pressed_cb cb, void* cb_args) {
	debounce_t ret;
	if (idx > 4) {
		ret.id = -1;
		ret.change = UNCHANGED;
		ret.pressed = 0;
	} else {
		debounce_data[idx].args = cb_args;
		debounce_data[idx].is_pressed = cb;
		debounce_data[idx].id = idx;
		debounce_data[idx].ticks = 0;
		debounce_data[idx].max_ticks = interval;

		ret.id = idx++;
		ret.change = UNCHANGED;
		ret.pressed = 0;
	}

	return ret;
}

void debounce_update(debounce_t* debounce) {
	debounce_data_t* data = &debounce_data[debounce->id];

	debounce->pressed = 0;
	debounce->change = UNCHANGED;
	int pressed = 0;
	switch (data->state) {
	case WAITING_PUSH:
		pressed = data->is_pressed(data->args);
		if (pressed) {
			data->state = NOISE_PUSH;
			data->ticks = data->max_ticks;
		}
		break;
	case NOISE_PUSH:
		if (data->ticks >= 0) {
			data->ticks--;
		} else if (data->is_pressed(data->args)) {
			data->state = PUSHING;

			debounce->change = FELL;
			debounce->pressed = 1;
		} else {
			data->state = WAITING_PUSH;
		}
		break;
	case PUSHING:
		if (!data->is_pressed(data->args)) {
			data->ticks = data->max_ticks;
			data->state = NOISE_RELEASE;
		} else {
			debounce->change = UNCHANGED;
			debounce->pressed = 1;
		}
		break;
	case NOISE_RELEASE:
		if (data->ticks > 0) {
			data->ticks--;
		} else if (data->is_pressed(data->args)) {
			data->state = PUSHING;
		} else {
			// liberaron el boton
			data->state = WAITING_PUSH;

			debounce->change = ROSE;
			debounce->pressed = 0;
		}
		break;
	}
}
