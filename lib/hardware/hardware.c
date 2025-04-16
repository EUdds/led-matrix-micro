#include "hardware.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

#define LOGGER_TAG "HARDWARE"
#define LOGI(...) ESP_LOGI(LOGGER_TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(LOGGER_TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(LOGGER_TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(LOGGER_TAG, __VA_ARGS__)
#define LOGV(...) ESP_LOGV(LOGGER_TAG, __VA_ARGS__)


#define POT_ADC_CHANNEL ADC1_CHANNEL_3 // VN pin
#define POT_ADC_WIDTH ADC_WIDTH_BIT_12 // 12 bits

static const float EMA_ALPHA = 0.2f; //0.1 - 0.3 work best


static uint8_t potentiometer_value = 0; // Variable to store potentiometer value

static void hardware_init(void)
{
    // Initialize ADC for potentiometer
    adc1_config_width(POT_ADC_WIDTH);
    adc1_config_channel_atten(POT_ADC_CHANNEL, ADC_ATTEN_DB_12); // Set attenuation to 11 dB
    LOGI("ADC initialized for potentiometer on channel %d", POT_ADC_CHANNEL);
}

static float smooth_ema(float new_value)
{
    static float last_value = 0.0f;
    last_value = (EMA_ALPHA * new_value) + ((1 - EMA_ALPHA) * last_value); // Exponential moving average
    return last_value; // Return the smoothed value
}


uint8_t hardware_getPotentiometerValue(void)
{
    return potentiometer_value; // Return the potentiometer value (0-100%)
}

float hardware_getPotentiometerValuef(void)
{
    return smooth_ema((float)(potentiometer_value) / 100.0f); // Return the smoothed potentiometer value as a float (0.0-1.0)
    // return (float)potentiometer_value / 100.0f; // Return the potentiometer value as a float (0.0-1.0)
}

void hardware_task(void* pvParameters)
{
    hardware_init();

    while(1)
    {
        int raw_value = adc1_get_raw(POT_ADC_CHANNEL); // Read raw ADC value
        int32_t sum = 0;
        const int samples = 10;
        for (int i = 0; i < samples; i++) {
            sum += adc1_get_raw(POT_ADC_CHANNEL); // Read raw ADC value
        }
        raw_value = sum / samples; // Average the samples
        if (raw_value < 0) {
            raw_value = 0; // Ensure raw value is not negative
        } else if (raw_value > 4095) {
            raw_value = 4095; // Ensure raw value does not exceed ADC max value
        }
        potentiometer_value = (uint8_t)((raw_value * 100) / 4095); // Convert to percentage (0-100%)

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}