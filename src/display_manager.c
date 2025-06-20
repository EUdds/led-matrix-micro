#include "display_manager.h"

#include "neopixel_driver.h"
#include "hardware.h"
#include "graphics.h"
#include "utils.h"
#include "clock.h"
#include "http_manager.h"
#include "genealogy.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "telnet_log.h"
#include "esp_heap_caps.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#define TAG "DISPLAY_MANAGER"

typedef struct {
    displayManager_buffer_t* buffers[MAX_DISPLAY_BUFFERS];
    uint32_t num_buffers;
    uint32_t* output_buffer;
    bool initialized;
} display_manager_ctx_t;

static display_manager_ctx_t dm_ctx = {0};
static const displayManager_rotation_E rotation = DISPLAY_ROTATION_270;
static const bool enablePotMonitoring = true;
static float current_brightness = 1.0f; // Default brightness


void display_manager_setBrightness(float brightness)
{
    if (brightness < 0.0f || brightness > 1.0f) {
        // Handle invalid brightness value
        return;
    }
    
    if (brightness != current_brightness) {
        current_brightness = brightness;
        // LOGI("Brightness set to: %.2f", current_brightness);
        neopixel_driver_setBrightness(brightness);
        // if (genealogy_set_brightness(brightness) != ESP_OK) {
        //     LOGE("Failed to set brightness in NVS");
        // } else {
        //     LOGI("Brightness saved to NVS: %.2f", brightness);
        // }
    }
}

void display_manager_setBufferPixel(displayManager_buffer_t* buffer, 
                                          uint32_t x, 
                                          uint32_t y, 
                                          uint32_t color)
{
    if (!buffer || !buffer->active) {
        LOGE("Invalid or inactive buffer");
        return;
    }

    if (x >= buffer->width || y >= buffer->height) {
        LOGE("Coordinates out of bounds: (%lu, %lu) for buffer size (%lu, %lu)", 
             x, y, buffer->width, buffer->height);
        return;
    }

    // Set pixel in the buffer
    buffer->buffer[y * buffer->width + x] = color;
}

void display_manager_setRawPixel(uint32_t row, uint32_t col, uint32_t color)
{
    uint32_t pixelIndex = 0;
    switch (rotation)
    {
        case DISPLAY_ROTATION_270:
            // Input cable is bottom right. Moving right increases column, moving down increses row
            // (0, 0) is 248
            // 0 is (7, 31)
            if (col % 2 == 0) {
                pixelIndex = (NEOPIXEL_NUM_COLS - 1 - col) * NEOPIXEL_NUM_ROWS + row;
            } else {
                pixelIndex = (NEOPIXEL_NUM_COLS - 1 - col) * NEOPIXEL_NUM_ROWS + (NEOPIXEL_NUM_ROWS - 1 - row);
            }

            break;
        
        default:
            LOGE("Invalid rotation value: %d", rotation);
            break;
    }
    neopixel_driver_setPixel(pixelIndex, color);
    // LOGD("Setting pixel at (%ld, %ld) to color %06lX (%lu)", row, col, color, pixelIndex);
}

void display_manager_clearScreen(void)
{
    neopixel_driver_clearMatrix();
}


esp_err_t display_manager_init(void)
{
    if (dm_ctx.initialized) {
        return ESP_OK;
    }

    // Allocate output buffer
    LOGD("Allocating output buffer of size %u bytes", 
         NEOPIXEL_NUM_ROWS * NEOPIXEL_NUM_COLS * sizeof(uint32_t));
    dm_ctx.output_buffer = heap_caps_calloc(NEOPIXEL_NUM_ROWS * NEOPIXEL_NUM_COLS, 
                                          sizeof(uint32_t), 
                                          MALLOC_CAP_8BIT);
    if (!dm_ctx.output_buffer) {
        LOGE("Failed to allocate output buffer");
        return ESP_ERR_NO_MEM;
    }

    dm_ctx.initialized = true;
    LOGI("Display manager initialized");
    return ESP_OK;
}

displayManager_buffer_t* display_manager_create_buffer(const char* owner_name, 
                                                     uint32_t width, 
                                                     uint32_t height,
                                                     uint32_t x,
                                                     uint32_t y,
                                                     displayManager_layer_E layer)
{
    if (!dm_ctx.initialized || dm_ctx.num_buffers >= MAX_DISPLAY_BUFFERS) {
        ESP_LOGE(TAG, "Display manager not initialized or maximum buffers reached");
        return NULL;
    }

    displayManager_buffer_t* buffer = heap_caps_calloc(1, 
                                                     sizeof(displayManager_buffer_t), 
                                                     MALLOC_CAP_8BIT);
    if (!buffer) {
        return NULL;
    }

    buffer->buffer = heap_caps_calloc(width * height, 
                                    sizeof(uint32_t), 
                                    MALLOC_CAP_8BIT);

    memset(buffer->buffer, TRANSPARENT, width * height * sizeof(uint32_t)); // Initialize buffer to transparent
    if (!buffer->buffer) {
        free(buffer);
        return NULL;
    }

    buffer->width = width;
    buffer->height = height;
    buffer->x = x;
    buffer->y = y;
    buffer->layer = layer;
    buffer->active = true;
    buffer->opacity = 255;
    buffer->owner = owner_name;

    dm_ctx.buffers[dm_ctx.num_buffers++] = buffer;
    return buffer;
}

static void merge_buffers(void)
{
    // Clear output buffer
    memset(dm_ctx.output_buffer, 0, NEOPIXEL_NUM_ROWS * NEOPIXEL_NUM_COLS * sizeof(uint32_t));

    // Sort buffers by layer
    for (displayManager_layer_E layer = DISPLAY_MANAGER_LAYER_BACKGROUND; 
         layer <= DISPLAY_MANAGER_LAYER_SYSTEM; 
         layer++) {
        
        // Process each buffer in the current layer
        for (uint32_t i = 0; i < dm_ctx.num_buffers; i++) {
            displayManager_buffer_t* buf = dm_ctx.buffers[i];
            if (!buf->active || buf->layer != layer) {
                continue;
            }

            // Copy buffer contents to output buffer with position offset
            for (uint32_t y = 0; y < buf->height; y++) {
                for (uint32_t x = 0; x < buf->width; x++) {
                    uint32_t display_x = buf->x + x;
                    uint32_t display_y = buf->y + y;
                    
                    if (display_x >= NEOPIXEL_NUM_COLS || display_y >= NEOPIXEL_NUM_ROWS) {
                        continue;
                    }

                    uint32_t color = buf->buffer[y * buf->width + x];
                    if (color != TRANSPARENT)
                    {
                        dm_ctx.output_buffer[display_y * NEOPIXEL_NUM_COLS + display_x] = color;
                    }
                }
            }
        }
    }
}

void display_manager_task(void* pvParameter)
{
    LOGI("Display Manager Task started");
    bool wifiStatusLedState = false;

    // if (genealogy_get_brightness(&current_brightness) != ESP_OK) {
    //     LOGE("Failed to get brightness from NVS");
    //     current_brightness = 1.0f; // Default to full brightness
    // }

    LOGI("Display Manager Task started with brightness: %.2f", current_brightness);
    while (1) {
        if (enablePotMonitoring) {
            // Read the potentiometer value and set the brightness accordingly
                static float lastPotValue = 0.0f;
                float potValue = hardware_getPotentiometerValuef(); // Get the potentiometer value (0-100%)
                display_manager_setBrightness(potValue); // Set brightness (0.0-1.0)
                if (abs((int)(potValue* 100) - (int)(lastPotValue * 100)) > 2) {
                    LOGI("Brightness Set: %.2f", potValue);
                    lastPotValue = potValue;
                }
        }

        if (dm_ctx.initialized) {
            merge_buffers();
            
            // Update the physical display using the merged buffer
            for (uint32_t i = 0; i < NEOPIXEL_NUM_ROWS * NEOPIXEL_NUM_COLS; i++) {
                // Use existing setPixel function to update the physical display
                uint32_t row = i / NEOPIXEL_NUM_COLS;
                uint32_t col = i % NEOPIXEL_NUM_COLS;
                if (dm_ctx.output_buffer[i] == TRANSPARENT)
                {
                    display_manager_setRawPixel(row, col, BLACK);
                }
                display_manager_setRawPixel(row, col, dm_ctx.output_buffer[i]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30fps refresh rate
    }
}