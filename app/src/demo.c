/*

 File: 		demo.c
 Author:	André van Schoubroeck
 License:	MIT


 MIT License

 Copyright (c) 2017, 2018, 2019, 2020 André van Schoubroeck

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
 * WS2812 DMA library demo
 */

#include <stdint.h>
#include "ws2812.h"

#include <zephyr/kernel.h>

#include <stdio.h>

static uint16_t m_speed = 100;
static bool m_direction = true;

uint16_t get_speed() {
	return m_speed;
}

void set_speed(uint16_t speed) {
	m_speed = speed;
}


bool get_direction() {
	return m_direction;
}

void set_direction(bool direction) {
	m_direction = direction;
}

void delay_ms(uint32_t milliseconds) {
		k_sleep(K_MSEC(milliseconds));
}

#pragma pack (push,1)
typedef struct {
	uint8_t g;
	uint8_t r;
	uint8_t b;
} rgb_t;
#pragma pack (pop)

#define RED (rgb_t){0,32,0}
#define ORANGE (rgb_t){16,32,0}
#define YELLOW (rgb_t){32,32,0}
#define GREEN (rgb_t){32,0,0}
#define CYAN (rgb_t){32,0,32}
#define BLUE (rgb_t){0,0,32}
#define PURPLE (rgb_t){0,32,32}
#define WHITE (rgb_t){16,16,16}

#define LED_COUNT (19)

/*
rgb_t m_colours[LED_COUNT]  = {
			{ 0x3F, 0x00, 0x00},
			{ 0x3F, 0x00, 0x14},
			{ 0x3F, 0x00, 0x28},
			{ 0x3F, 0x00, 0x3C},
			{ 0x2E, 0x00, 0x3F},
			{ 0x1A, 0x00, 0x3F},
			{ 0x06, 0x00, 0x3F},
			{ 0x00, 0x0D, 0x3F},
			{ 0x00, 0x21, 0x3F},
			{ 0x00, 0x35, 0x3F},
			{ 0x00, 0x3F, 0x35},
			{ 0x00, 0x3F, 0x21},
			{ 0x00, 0x3F, 0x0D},
			{ 0x06, 0x3F, 0x00},
			{ 0x1A, 0x3F, 0x00},
			{ 0x2E, 0x3F, 0x00},
			{ 0x3F, 0x3C, 0x00},
			{ 0x3F, 0x28, 0x00},
			{ 0x3F, 0x14, 0x00},
};
*/


volatile static rgb_t m_colours[LED_COUNT]  = {
			{ 0x3F, 0x00, 0x00},
			{ 0x3F, 0x00, 0x14},
			{ 0x3F, 0x00, 0x28},
			{ 0x3F, 0x00, 0x3C},
			{ 0x2E, 0x00, 0x3F},
			{ 0x1A, 0x00, 0x3F},
			{ 0x06, 0x00, 0x3F},
			{ 0x00, 0x0D, 0x3F},
			{ 0x00, 0x21, 0x3F},
			{ 0x00, 0x35, 0x3F},
			{ 0x00, 0x3F, 0x35},
			{ 0x00, 0x3F, 0x21},
			{ 0x00, 0x3F, 0x0D},
			{ 0x06, 0x3F, 0x00},
			{ 0x1A, 0x3F, 0x00},
			{ 0x2E, 0x3F, 0x00},
			{ 0x3F, 0x3C, 0x00},
			{ 0x3F, 0x28, 0x00},
			{ 0x3F, 0x14, 0x00},
};



void get_leds(void**leds, int *size) {
	*leds=(void*)(m_colours+1);
	*size=(LED_COUNT * sizeof(rgb_t))-sizeof(rgb_t);
}


void ws2812_demo() {
//	rgb_t m_colours[LED_COUNT]  = {
//		RED,ORANGE,YELLOW,   // 3
//		GREEN,CYAN,BLUE,     // 6
//		PURPLE,RED, ORANGE,  // 9
//		YELLOW, GREEN,CYAN, // 12
//		BLUE, PURPLE, RED,  // 15
//		YELLOW, GREEN, BLUE, // 18
//		PURPLE, // 19
//		};


// So it works defined here, but not globally????

	rgb_t temp;

	puts("Starting Demo");

	//test
	static int count = 0;

	while (1) {
		//while (ws2812_is_busy());
		ws2812_fill_buffer_decompress(0, sizeof(m_colours), (uint8_t *)&m_colours);


		// so if I put a breakpoint between decompress and apply, and let it run again
		// it magically starts working.
		//__DMB();
		//__DSB();
		//__ISB();
		//delay_ms(1);
		//k_yield();
		// None of these appear to work. 
		// This was all with gcc 14 
		// using the gcc 9 compiler appears to work???

//		testing changing direction
//		count++;
//		if (!(count%42)) m_direction = ! m_direction;


		ws2812_apply(sizeof(m_colours));
		if (m_speed) {
			delay_ms(m_speed);

//				count++;
//				if (!(count%42)) m_direction = ! m_direction;

			if (m_direction) {
				m_colours[0] = ORANGE;
				temp=m_colours[1];
				for (int i = 1; i < (LED_COUNT-1); i++) {
					m_colours[i]=m_colours[i+1];
				}
				m_colours[(LED_COUNT-1)]=temp;
			} else {
				m_colours[0] = CYAN;
				temp=m_colours[LED_COUNT-1];
				for (int i = LED_COUNT-1; i > (1); i--) {
					m_colours[i]=m_colours[i-1];
				}
				m_colours[1]=temp;
			}

		} else {
			delay_ms(1000);
		}
	}
}



