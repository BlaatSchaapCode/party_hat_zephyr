/*

 File: 		ws2812_stm32.c
 Author:	André van Schoubroeck
 License:	MIT


 MIT License

 Copyright (c) 2017, 2018, 2019, 2020  André van Schoubroeck <andre@blaatschaap.be>

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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_tim.h"

volatile bool buffer_state[3];
volatile bool timer_state[3];

uint8_t ws2812_data[3104 * 4]; // 4 Clockless channels of either 96 RGBW or 128 RGB leds

void ws2812_tim2_init() {
	GPIO_InitTypeDef GPIO_InitStruct;

	// Common configuration for all channels
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	// Enable Timer 2 Clock
	__HAL_RCC_TIM2_CLK_ENABLE();

	// Enable GPIO Port A Clock
	__HAL_RCC_GPIOA_CLK_ENABLE();

	// Pins for timer 2

	// Apply pin configuration to PA0
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Apply pin configuration to PA1
	GPIO_InitStruct.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Apply pin configuration to PA2
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Apply pin configuration to PA3
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void ws2812_tim3_init() {

	GPIO_InitTypeDef GPIO_InitStruct;

	// Common configuration for all channels
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	// Enable Timer 3 Clock
	__HAL_RCC_TIM3_CLK_ENABLE();

	// Enable GPIO Port A Clock
	__HAL_RCC_GPIOA_CLK_ENABLE();

	// Enable GPIO Port B Clock
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// Pins for timer 3
	// Apply pin configuration to PA6
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Apply pin configuration to PA7
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Apply pin configuration to PB0
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Apply pin configuration to PB1
	GPIO_InitStruct.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void ws2812_pins_init() {
	ws2812_tim2_init();
	ws2812_tim3_init();
}

void ws2812_set_xchannels(TIM_TypeDef *tim, int firstChannel, int nrChannels) {
	if (firstChannel + nrChannels <= 4) {
		tim->CR1 &= ~0x0001; // disable
		__DSB();
		tim->DCR &= ~TIM_DCR_DBA_Msk;
		tim->DCR |= ((13 + firstChannel) << TIM_DCR_DBA_Pos); // DMA Transfer Base address CCR1 + channel (0-3)
		tim->DCR &= ~TIM_DCR_DBL_Msk;
		tim->DCR |= ((nrChannels - 1) << TIM_DCR_DBL_Pos); // nr of transfers a time (CCR1 to CCR4)
		tim->CR1 |= 0x0001; // enable
		__DSB();
	}
}

void ws2812_set_4channel(TIM_TypeDef *tim) {
	ws2812_set_xchannels(tim, 0, 4);
}

void ws2812_init2(DMA_Channel_TypeDef *dma, TIM_TypeDef *tim) {
	dma->CPAR = (uint32_t) & (tim->DMAR); // Link DMA channel to timer

	dma->CCR = 0x00;
	dma->CCR |= (0x01 << DMA_CCR_PSIZE_Pos); // Peripheral size 16 bit
	dma->CCR |= (0x00 << DMA_CCR_MSIZE_Pos); // Memory size 8 bit
	dma->CCR |= (0x1 << DMA_CCR_DIR_Pos);   // Memory to Peripheral
	dma->CCR |= (0x1 << DMA_CCR_MINC_Pos);   // Memory increasement
	dma->CCR |= (0x0 << DMA_CCR_PINC_Pos);  // Peripheral increasement
	dma->CCR |= (0x0 << DMA_CCR_CIRC_Pos);   // Circular mode
	dma->CCR |= DMA_CCR_TCIE; // Enable transfer complete interrupt

	dma->CCR |= DMA_CCR_TEIE; // Enable transfer error interrupt

	// I have used these values in the old code. However, I can't derive them
	// 72 MHz / (9+1) = 7.2 MHz
	// 7.2 MHz / 8 = 900 kHz.
	// We should have 800 kHz, shouldn't we? So why did this work?
	//tim->PSC = 9; // Prescaler. 
	//tim->ARR = 8; // Reload Value

	// We need to generate a signal on 800 kHz
	//tim->PSC = 8; // Prescaler: 72 MHZ / (8+1) = 8 MHz

	// Calculate the prescaler by reading the current core speed rather then
	// assuming its configured to 72 MHz
	tim->PSC = (SystemCoreClock / 8000000) - 1;
	tim->ARR = 10 - 1; // Reload Value 8 MHz / 10 = 800 kHz

	// DMA will be configured when we initiate a transfer. At this point,
	// clear all DMA related settings

	// Disable DMA.
	tim->DIER = 0;
	// Clear DMA settings.
	tim->DCR = 0;

	// Configure each channel for PWM
	tim->CCMR1 = 0;
	tim->CCMR1 |= (0b110 << TIM_CCMR1_OC1M_Pos); // pwm mode 1
	tim->CCMR1 |= (0b110 << TIM_CCMR1_OC2M_Pos); // pwm mode 1

	tim->CCMR2 = 0;
	tim->CCMR2 |= (0b110 << TIM_CCMR2_OC3M_Pos); // pwm mode 1
	tim->CCMR2 |= (0b110 << TIM_CCMR2_OC4M_Pos); // pwm mode 1

	tim->CCMR1 |= TIM_CCMR1_OC1PE; // output compare preload enable
	tim->CCMR1 |= TIM_CCMR1_OC2PE; // output compare preload enable

	tim->CCMR2 |= TIM_CCMR2_OC3PE; // output compare preload enable
	tim->CCMR2 |= TIM_CCMR2_OC4PE; // output compare preload enable

	// Set Output Enable for all Channels
	tim->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	tim->CR1 |= 1 << 7; // auto reload enable
	tim->CR1 &= ~(0b1110000); // Edge aglined, upcounting
	tim->CR1 |= 0b100; // Event source, only over/underflow
}

void ws2812_start_dma_transer2(void *memory, size_t size,
		DMA_Channel_TypeDef *dma, TIM_TypeDef *tim) {

	__disable_irq();
	buffer_state[0] = true;
	__enable_irq();

	switch ((uint32_t) tim) {
	case (uint32_t) TIM2:
		if (timer_state[0]) {
			return; // BUSY // TODO GENERATE ERROR MESSAGE
		} else
			timer_state[0] = true;
		break;
	case (uint32_t) TIM3:
		if (timer_state[1]) {
			return; // BUSY // TODO GENERATE ERROR MESSAGE
		} else
			timer_state[1] = true;
		break;
	case (uint32_t) TIM4:
		if (timer_state[2]) {
			return; // BUSY // TODO GENERATE ERROR MESSAGE
		} else
			timer_state[2] = true;
	}

	tim->DIER = TIM_DIER_UDE; // Update DMA Request Enable
	dma->CNDTR = size;
	dma->CMAR = (uint32_t) memory;
	tim->CCMR1 |= 1; // enable timer
	dma->CCR |= 1; // Enable DMA
	tim->EGR = TIM_EGR_UG; // Trigger update event, starting the transfer
}
void ws2812_start_dma_transer(char *memory, size_t size) {
	ws2812_start_dma_transer2(memory, size, DMA1_Channel2, TIM2);
}

void ws2812_apply(size_t size) {
	ws2812_start_dma_transer2(ws2812_data, 32 + (size * 8), DMA1_Channel2, TIM2);
	memset(ws2812_data+ (size*8),0,32); // extra reset
}

void ws2812_apply_channel(size_t size, int channel) {
	DMA_Channel_TypeDef *dma;
	TIM_TypeDef *tim;
	switch (channel) {
	case 0:
	case 1:
	case 2:
	case 3:
		dma = DMA1_Channel2;
		tim = TIM2;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		dma = DMA1_Channel3;
		tim = TIM3;
		break;
	default:
		return;
	}
	ws2812_set_xchannels(tim, channel % 4, 1);
	memset(ws2812_data+ (size*8),0,32); // extra reset
	ws2812_start_dma_transer2(ws2812_data, 32 + (size * 8), dma, tim);


}

void ws2812_init() {

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable timer2 clock
	RCC->AHBENR |= RCC_AHBENR_DMA1EN; // enable dma1 clock

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // enable timer3 clock

	ws2812_pins_init();

	// Datasheet page 281
	// TIM1_UP --> DMA 1 Channel 5
	// TIM2_UP --> DMA 1 Channel 2
	// TIM3_UP --> DMA 1 Channel 3
	// TIM4_UP --> DMA 1 Channel 7
	ws2812_init2(DMA1_Channel2, TIM2);
	ws2812_init2(DMA1_Channel3, TIM3);
	// We could also add Timer 4, then we have 4 more channels but no I²C
	// For Timer 1, we could add 2 channels, the other channels are in use by USB

	NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	NVIC_ClearPendingIRQ(DMA1_Channel3_IRQn);
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);

	NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_EnableIRQ(TIM2_IRQn);

	NVIC_ClearPendingIRQ(TIM3_IRQn);
	NVIC_EnableIRQ(TIM3_IRQn);

	// Prepare the data buffer to turn off all LEDs
	memset(ws2812_data, 2, sizeof(ws2812_data));

	/*
	 // Configure TIM2 and TIM3 for 4-channel output and turn off the LEDs
	 pwm_set_4channel(TIM2);
	 pwm_set_4channel(TIM3);
	 start_dma_transer2(data_c0, sizeof(data_c0), DMA1_Channel2, TIM2);
	 start_dma_transer2(data_c0, sizeof(data_c0), DMA1_Channel3, TIM3);
	 */

	// Configure TIM2 for single channel output and turn off the LEDs
	ws2812_set_xchannels(TIM2, 0, 1);
	ws2812_start_dma_transer2(ws2812_data, sizeof(ws2812_data), DMA1_Channel2,
			TIM2);

}

bool ws2812_is_busy2(DMA_Channel_TypeDef *dma) {
	return dma->CCR & 1;
}

bool ws2812_is_busy() {

	//return is_busy2(DMA1_Channel2) || is_busy2(DMA1_Channel3) ;

	return timer_state[0] || timer_state[1] || buffer_state[0];
}

void DMA1_Channel2_IRQHandler(void) {
	DMA1->IFCR = DMA1->ISR;
	DMA1_Channel2->CCR &= ~1; // Disable DMA

	// Resetting channel state
	TIM2->CCR1 = 0;
	TIM2->CCR2 = 0;
	TIM2->CCR3 = 0;
	TIM2->CCR4 = 0;

	TIM2->RCR = 32; // Keep reset time for 32 bits (RGBW)
	TIM2->DIER = TIM_DIER_UIE; // Update Interrupt Enable

	__disable_irq();
	buffer_state[0] = false;
	__enable_irq();
}

void DMA1_Channel3_IRQHandler(void) {
	DMA1->IFCR = DMA1->ISR;
	DMA1_Channel3->CCR &= ~1; // Disable DMA

	// Resetting channel state
	TIM3->CCR1 = 0;
	TIM3->CCR2 = 0;
	TIM3->CCR3 = 0;
	TIM3->CCR4 = 0;

	TIM3->RCR = 32; // Keep reset time for 32 bits (RGBW)
	TIM3->DIER = TIM_DIER_UIE; // Update Interrupt Enable

	__disable_irq();
	buffer_state[0] = false;
	__enable_irq();

}

void DMA1_Channel7_IRQHandler(void) {
	DMA1->IFCR = DMA1->ISR;
	DMA1_Channel7->CCR &= ~1; // Disable DMA

	// Resetting channel state
	TIM4->CCR1 = 0;
	TIM4->CCR2 = 0;
	TIM4->CCR3 = 0;
	TIM4->CCR4 = 0;

	__disable_irq();
	buffer_state[0] = false;
	__enable_irq();

}

void TIM2_IRQHandler(void) {
	timer_state[0] = false;
	TIM2->DIER = 0;
}

void TIM3_IRQHandler(void) {
	timer_state[1] = false;
	TIM3->DIER = 0;
}

/*
void TIM4_IRQHandler(void) {
	timer_state[2] = false;
	TIM4->DIER = 0;
}
*/
