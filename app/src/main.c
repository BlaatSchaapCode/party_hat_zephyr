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

int main(void) {
	ble_init();
	ws2812_demo();
 

	return 0;
}

