#include "neopixel_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "driver/gpio.h"
#include <neopixel.h>
#include "esp_log.h"
#include "esp_timer.h"

#ifndef NEOPIXEL_GPIO
#define NEOPIXEL_GPIO (GPIO_NUM_27)
#endif

#ifndef NEOPIXEL_NUM_LEDS
#define NEOPIXEL_NUM_LEDS (256)
#endif

#define QUEUE_BUFFER_SIZE (NEOPIXEL_NUM_LEDS * 3)

#define LOGGER_TAG "NEOPIXEL_DRIVER"

#define LOGI(...) ESP_LOGI(LOGGER_TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(LOGGER_TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(LOGGER_TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(LOGGER_TAG, __VA_ARGS__)

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define BRIGHTNESS_SCALE 256
#define MAX_BRIGHTNESS 200 // May need some tuneing



static tNeopixel pixelBuffer[NEOPIXEL_NUM_LEDS] = {0};
static QueueHandle_t pixelQueue = NULL;
static tNeopixelContext neopixel = NULL;


static float brightness = 0.0f;


static uint32_t refreshRate = 500; // Ticks

static int neopixel_driver_init(void)
{
    // neopixel = neopixel_Init(NEOPIXEL_NUM_LEDS, NEOPIXEL_GPIO);
    neopixel = neopixel_Init(NEOPIXEL_NUM_LEDS, GPIO_NUM_27);
    if (neopixel == NULL) 
    {
        LOGE("Failed to initialize NeoPixel driver");
        return -1;
    }
    
    refreshRate = neopixel_GetRefreshRate(neopixel);
    refreshRate = MAX(1, pdMS_TO_TICKS( 1000UL/ refreshRate)); // Convert to milliseconds
    LOGI("NeoPixel refresh rate: %lu", pdTICKS_TO_MS(refreshRate));
    pixelQueue = xQueueCreate(QUEUE_BUFFER_SIZE, sizeof(tNeopixel));
    return 0;
}

// Apply brightness to the input buffer and store the result in the output buffer
static void neopixel_driver_applyBrightness(tNeopixel* input, tNeopixel* output)
{
    uint8_t bright = (uint8_t)(brightness * BRIGHTNESS_SCALE);
    bright = MIN(bright, MAX_BRIGHTNESS); // Ensure brightness does not exceed max
    for (uint32_t i = 0; i < NEOPIXEL_NUM_LEDS; i++) {

        uint8_t r = (input[i].rgb >> 16) & 0xFF;
        uint8_t g = (input[i].rgb >> 8) & 0xFF;
        uint8_t b = input[i].rgb & 0xFF;

        // Apply brightness scaling
        r = (r * bright) / BRIGHTNESS_SCALE;
        g = (g * bright) / BRIGHTNESS_SCALE;
        b = (b * bright) / BRIGHTNESS_SCALE;

        output[i].rgb = (r << 16) | (g << 8) | b;
        output[i].index = input[i].index; // Keep the index the same
    }
}


void neopixel_driver_setBrightness(float b)
{
    if (b < 0.0f || b > 1.0f) {
        LOGE("Brightness must be between 0.0 and 1.0");
        return;
    }
    brightness = b;
    LOGD("Setting brightness to: %.2f", brightness);
}

void neopixel_driver_addToQueue(tNeopixel* pixel, uint32_t pixelCount)
{
    if (pixelQueue == NULL) 
    {
        LOGE("Pixel queue is not initialized");
        return;
    }
    
    for (uint32_t i = 0; i < pixelCount; i++) {
        if (xQueueSend(pixelQueue, &pixel[i], portMAX_DELAY) != pdTRUE) {
            LOGE("Failed to send pixel to queue");
        } else {
            LOGD("Pixel added to queue: index=%lu, color=%06lX", pixel[i].index, pixel[i].rgb);
        }
    }
}

void neopixel_driver_setRawPixel(tNeopixel* pixel)
{
    LOGI("Setting raw pixel: index=%lu, color=%06lX", pixel->index, pixel->rgb);
    pixelBuffer[pixel->index] = *pixel;
}

void neopixel_driver_setPixel(int index, uint32_t color)
{
    if (index < 0 || index >= NEOPIXEL_NUM_LEDS) {
        LOGE("Index out of bounds: %d", index);
        return;
    }
    pixelBuffer[index].rgb = color;
    pixelBuffer[index].index = index;
}

void neopixel_driver_fill_matrix(uint32_t rgb)
{
    for (uint32_t i = 0; i < NEOPIXEL_NUM_LEDS; i++) {
        pixelBuffer[i].index = i;
        pixelBuffer[i].rgb = rgb;
    }
}

void neopixel_task(void* pvParameter)
{
    if (neopixel_driver_init())
    {
        LOGE("Failed to initialize NeoPixel driver");
        return;
    }

    int64_t prevTime = esp_timer_get_time() / 1000;
    int64_t currTime = esp_timer_get_time() / 1000;
    while(1)
    {
        currTime = esp_timer_get_time() / 1000;
        prevTime = currTime;
        tNeopixel correctedPixelBuffer[NEOPIXEL_NUM_LEDS] = {0};
        neopixel_driver_applyBrightness(pixelBuffer, correctedPixelBuffer);
        if (!neopixel_SetPixel(neopixel, correctedPixelBuffer, NEOPIXEL_NUM_LEDS)) {
            LOGE("Failed to set pixel color");
        }
        vTaskDelay(refreshRate); // Delay to control refresh rate
    }
}