/*

File: 		ws2812_nrfx.h
Author:		André van Schoubroeck
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

#ifndef DRIVERS_WS2812_NRFX_H_
#define DRIVERS_WS2812_NRFX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>



// https://devzone.nordicsemi.com/f/nordic-q-a/26371/pwm-driver---invert-pin/103775#103775
#define WS2812_VAL_0 0x8002
#define WS2812_VAL_1 0x8008


#define pwm_len		(3072 * 4)  // 4 Clockless channels of either 96 RGBW or 128 RGB leds
#define reset_time	(32 * 8)


extern uint16_t ws2812_data[pwm_len + 1];


// On the nRF52 series, we can output the signal on any pin
// The default pins are set to match the pinout on the boards to the stm32
// for TIM2.
#ifndef WS2812_PIN0
#define WS2812_PIN0 (3)
#endif

#ifndef WS2812_PIN1
#define WS2812_PIN1 (26)
#endif

#ifndef WS2812_PIN2
#define WS2812_PIN2 (27)
#endif

#ifndef WS2812_PIN3
#define WS2812_PIN3 (28)
#endif


bool ws2812_is_busy();
void ws2812_apply(size_t size);

#endif /* DRIVERS_WS2812_NRFX_H_ */
