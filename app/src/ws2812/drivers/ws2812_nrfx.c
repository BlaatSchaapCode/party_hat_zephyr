/*

 File: 		ws2812_nrfx.c
 Author:	André van Schoubroeck
 License:	MIT


 MIT License

 Copyright (c) 2018, 2019 André van Schoubroeck

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 */

/*
 *  For the moment this is only a single channel implementation
 *  TODO: Multi Channel implementation
 *
 */

#include "nrfx_pwm.h"
#include "hal/nrf_gpio.h"

#include "ws2812_nrfx.h"

nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(0);

volatile uint16_t ws2812_data[pwm_len + 32];

nrf_pwm_sequence_t seq = { .values.p_raw = ws2812_data, .length = pwm_len + 32,
		.repeats = 0, .end_delay = 32 };

static volatile bool m_busy = false;

static void pwm_handler(nrfx_pwm_evt_type_t event_type, void *context) {
	if (event_type == NRFX_PWM_EVT_FINISHED) {
		m_busy = false;
//		nrfx_pwm_uninit(&pwm);
//		nrf_gpio_pin_clear(WS2812_PIN0);
	}
}

void ws2812_init() {
	nrfx_pwm_config_t config;
	config.base_clock = NRF_PWM_CLK_8MHz;
	config.top_value = 10; // clock = 8 MHz, we need a 800 kHz signal

	config.count_mode = NRF_PWM_MODE_UP;
	config.irq_priority = 7;
	config.load_mode = NRF_PWM_LOAD_COMMON;
	// We should not indicate the channel polarisation here,
	// But only by setting bit 15 of the value.
	// Setting it here too will cause the first LED of the chain to fail
	config.output_pins[0] = WS2812_PIN0;	// | NRFX_PWM_PIN_INVERTED;
	config.output_pins[1] = config.output_pins[2] = config.output_pins[3] = -1;

	config.step_mode = NRF_PWM_STEP_AUTO;

	nrfx_pwm_uninit(&pwm);
	nrf_gpio_pin_clear(WS2812_PIN0);
	nrf_gpio_cfg_output(WS2812_PIN0);
	nrfx_pwm_init(&pwm, &config, pwm_handler, NULL);

}

bool ws2812_is_busy() {
	return m_busy;
}

void ws2812_apply(size_t size) {
	ws2812_init();

	// Zephyr!!!
	IRQ_CONNECT(PWM0_IRQn, IRQ_PRIO_LOWEST, nrfx_pwm_0_irq_handler, 0, 0);

	m_busy = true;

	if ((size * 8) < pwm_len) {
		// We repeat the last value...
		// In the previous use of this I've used value 10, why did I use 10
		// in stead of 0?
		//ws2812_data[(size * 8) + 1] = 10;
		//ws2812_data[(size * 8) ] = 10;
//		ws2812_data[(size * 8) ] = 0x8000;
//		seq.length = (size * 8) + 1;

		//nrfx_pwm_simple_playback(&pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
		nrfx_pwm_simple_playback(&pwm, &seq, 1, NRFX_PWM_FLAG_STOP);

		// le sigh. busy wait for the interrupt to occur, otherwise
		// the zephyr idle thread messes things up.
		// My guess it stops some clock we are depending on, as zephyr is 
		// not aware we are using the pwm peripheral through nrfx
		// (it is set to nrfx so zephyr won't touch it, but it does not know
		//  it is busy) 
		while (ws2812_is_busy())
			;
	}

}

