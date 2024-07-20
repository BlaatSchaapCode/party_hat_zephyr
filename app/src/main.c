/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app/drivers/blink.h>

#include <app_version.h>

#include <stdio.h>


#include "bat.h"


extern void ble_battery_process(struct k_timer *work);
K_TIMER_DEFINE(ble_battery_timer, ble_battery_process, NULL);


extern void ble_led_process(struct k_timer *work);
K_TIMER_DEFINE(ble_led_timer, ble_led_process, NULL);


extern void ws2812_demo(void);


int ble_init(void);

int main(void) {
	ble_init();
	adc_init();

	// Measure Battery Every Second
//	k_timer_start(&ble_battery_timer, K_SECONDS(1), K_SECONDS(1));


// Ain't working in a timer yet. PWM doesn't run, timer doesn't fire
	// Probably the Zephyr timer runs on a hardware timer interrupt
	// with higher priority then the PWM
//	k_timer_start(&ble_led_timer, K_MSEC(200),  K_MSEC(200));

//	ws2812_demo();
 

	return 0;
}

