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


void ws2812_demo(void);
int ble_init(void);

extern void ble_battery_process(struct k_timer *work);
extern void adc_init();
K_TIMER_DEFINE(ble_battery_timer, ble_battery_process, NULL);



int main(void) {
	ble_init();
	adc_init();

	// Measure Battery Every Second
	k_timer_start(&ble_battery_timer, K_SECONDS(1), K_SECONDS(1));

	ws2812_demo();
 

	return 0;
}

