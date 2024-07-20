#include "nrfx.h"


#include "nrfx_saadc.h"
#include "hal/nrf_saadc.h"



#define ADC_CHANNEL_COUNT (1)
//nrf_saadc_value_t g_adc_values[ADC_CHANNEL_COUNT];  // array of voids???
// is something wrong with the nrfx integration??? 
// Seems the nrfx version check macro is doing something wacky

uint16_t g_adc_values[ADC_CHANNEL_COUNT];  


void adc_init(void) {
    nrfx_saadc_init(6);
    nrfx_saadc_channel_t channel_configs[ADC_CHANNEL_COUNT];
    channel_configs[0] = (nrfx_saadc_channel_t)NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);
    channel_configs[0].channel_config.gain = NRF_SAADC_GAIN1_4;

    nrfx_saadc_channels_config(channel_configs, ADC_CHANNEL_COUNT);
    nrfx_saadc_simple_mode_set(0b1, NRF_SAADC_RESOLUTION_12BIT, NRF_SAADC_OVERSAMPLE_4X, NULL);
}

int adc_measure(void) {
    int result = -1;   
    nrfx_saadc_buffer_set(g_adc_values, ADC_CHANNEL_COUNT);
    if (NRFX_SUCCESS == nrfx_saadc_mode_trigger()) {
        result = 0;
    }

	float mv = (float)(*g_adc_values) / 4096.0f * 4800.0f;
    return mv;
}
