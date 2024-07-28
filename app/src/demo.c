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

#include "ws2812.h"
#include <stdint.h>
#include "ble.h"

#include <zephyr/kernel.h>

#include <stdio.h>

static volatile uint16_t m_speed = 100;
static volatile bool m_direction = true;

uint16_t get_speed() { return m_speed; }

void set_speed(uint16_t speed) { m_speed = speed; }

bool get_direction() { return m_direction; }

void set_direction(bool direction) { m_direction = direction; }

void delay_ms(uint32_t milliseconds) { k_sleep(K_MSEC(milliseconds)); }

#pragma pack(push, 1)
typedef struct {
  uint8_t g;
  uint8_t r;
  uint8_t b;
} rgb_t;
#pragma pack(pop)

#define RED                                                                    \
  (rgb_t) { 0, 32, 0 }
#define ORANGE                                                                 \
  (rgb_t) { 16, 32, 0 }
#define YELLOW                                                                 \
  (rgb_t) { 32, 32, 0 }
#define GREEN                                                                  \
  (rgb_t) { 32, 0, 0 }
#define CYAN                                                                   \
  (rgb_t) { 32, 0, 32 }
#define BLUE                                                                   \
  (rgb_t) { 0, 0, 32 }
#define PURPLE                                                                 \
  (rgb_t) { 0, 32, 32 }
#define WHITE                                                                  \
  (rgb_t) { 16, 16, 16 }

#define LED_COUNT (19)

volatile static rgb_t m_colours[LED_COUNT] = {
    {0x00, 0x00, 0x00}, {0x19, 0x00, 0x00}, {0x19, 0x08, 0x00},
    {0x19, 0x11, 0x00}, {0x19, 0x19, 0x00}, {0x10, 0x19, 0x00},
    {0x08, 0x19, 0x00}, {0x00, 0x19, 0x00}, {0x00, 0x19, 0x08},
    {0x00, 0x19, 0x11}, {0x00, 0x19, 0x19}, {0x00, 0x10, 0x19},
    {0x00, 0x08, 0x19}, {0x00, 0x00, 0x19}, {0x08, 0x00, 0x19},
    {0x11, 0x00, 0x19}, {0x19, 0x00, 0x19}, {0x19, 0x00, 0x10},
    {0x19, 0x00, 0x08},
};

void get_leds(void **leds, int *size) {
  *leds = (void *)(m_colours + 1);
  *size = (LED_COUNT * sizeof(rgb_t)) - sizeof(rgb_t);
}

void ws2812_blue() {
  rgb_t blue[LED_COUNT] = {
      BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE,
      BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLUE,
  };
  ws2812_fill_buffer_decompress(0, sizeof(blue), blue);
  ws2812_apply(sizeof(blue));
}

static volatile bool m_button_pressed;
void button_pressed(void) {
	m_button_pressed = true;
}

#include <zephyr/bluetooth/bluetooth.h>
void ws2812_demo() {
  rgb_t temp, status;
  puts("Starting Demo");

  while (1) {
	if (m_button_pressed) {
		m_button_pressed = false;

		if (ble_is_enabled()) {
			puts("Disable BLE");
			ble_disable();
		} else {
			puts("Enable BLE");
			ble_enable();
		}
	}



    int key;
    key = irq_lock();
    rgb_t copy_of_colours[LED_COUNT];
    memcpy(copy_of_colours, m_colours, sizeof(m_colours));



	if (ble_is_enabled()) {
		if (ble_is_connected()) {
			copy_of_colours[0] = BLUE;
		} else {
			copy_of_colours[0] = GREEN;
		}
	} else {
		copy_of_colours[0] = RED;
	}



    irq_unlock(key);

    ws2812_fill_buffer_decompress(0, sizeof(copy_of_colours), copy_of_colours);
    ws2812_apply(sizeof(copy_of_colours));

    if (m_speed) {

      if (m_direction) {
        temp = copy_of_colours[1];
        for (int i = 1; i < (LED_COUNT - 1); i++) {
          copy_of_colours[i] = copy_of_colours[i + 1];
        }
        copy_of_colours[(LED_COUNT - 1)] = temp;
      } else {

        temp = copy_of_colours[LED_COUNT - 1];
        for (int i = LED_COUNT - 1; i > (1); i--) {
          copy_of_colours[i] = copy_of_colours[i - 1];
        }
        copy_of_colours[1] = temp;
      }

 

      key = irq_lock();
      memcpy((void *)m_colours, copy_of_colours, sizeof(m_colours));
      irq_unlock(key);

      delay_ms(m_speed);
    } else {
      delay_ms(250);
    }
  }
}
