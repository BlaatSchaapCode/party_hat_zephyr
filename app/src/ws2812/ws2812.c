/*

 File: 		ws2812.c
 Author:		André van Schoubroeck
 License:	MIT


 MIT License

 Copyright (c) 2017, 2018, 2019, 2020 André van Schoubroeck <andre@blaatschaap.be>

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

#include <stdint.h>
#include <stdbool.h>

#include "driver.h"

// WS2812 PWM Timing
// We have configured our timer to 8 MHz, with a period of 10. This gives us a
// 800 kHz signal. These values give us a 20% DT for 0 and 80% DT for 1
static int pwm_val_0 = WS2812_VAL_0;
static int pwm_val_1 = WS2812_VAL_1;

// All known (to me) implementations of LEDS using an WS2812-style protocol use
// 8 bit per colour.
static int bits_per_colour = 8;

// This function converts byte data to per-bit values for a PWM (duty cycle)
// DMA transfer
bool ws2812_fill_buffer_decompress(uint16_t offset, uint16_t amount,
		uint8_t *data) {
	for (int count = 0; count < amount; count++)
		for (int bit_nr = 0; bit_nr < bits_per_colour; bit_nr++) {
			uint8_t byteval = data[count];
			int mask = 1 << bit_nr;
			int val = (byteval & mask) ? pwm_val_1 : pwm_val_0;
			uint32_t index = ((bits_per_colour - 1) - bit_nr)
					+ (8 * (offset + count));
			if (index > pwm_len)
				return false;
			ws2812_data[index] = val;
		}
	return true;
}

