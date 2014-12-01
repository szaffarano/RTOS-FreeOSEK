/*
 * debounce.h
 *
 *  Created on: 12/11/2014
 *      Author: sebas
 */

#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

typedef int (*is_pressed_cb)(void* args);

typedef enum _change_t {
	UNCHANGED, ROSE, FELL
} change_t;

typedef struct _debounce_t {
	int id;
	int pressed;
	change_t change;
} debounce_t;

debounce_t debounce_add(int interval, is_pressed_cb cb, void* cb_args);

void debounce_update(debounce_t*);

#endif /* DEBOUNCE_H_ */
