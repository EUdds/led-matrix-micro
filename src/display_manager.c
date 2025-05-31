#include "display_manager.h"

#include "neopixel_driver.h"
#include "hardware.h"
#include "graphics.h"
#include "utils.h"
#include "clock.h"
#include "http_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"

#include <stdint.h>


#define TAG "DISPLAY_MANAGER"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)

static const displayManager_rotation_E rotation = DISPLAY_ROTATION_270;
static const bool enablePotMonitoring = true;


void display_manager_setBrightness(float brightness)
{
    if (brightness < 0.0f || brightness > 1.0f) {
        // Handle invalid brightness value
        return;
    }
    neopixel_driver_setBrightness(brightness);
}

void display_manager_setPixel(uint32_t row, uint32_t col, uint32_t color)
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
    LOGD("Setting pixel at (%ld, %ld) to color %06lX (%lu)", row, col, color, pixelIndex);
}

void display_manager_clearScreen(void)
{
    neopixel_driver_clearMatrix();
}

void display_manager_task(void* pvParameter)
{
    clock_datetime_t time = {0};
    bool wifiStatusLedState = false;

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
        if (get_current_time12(&time)) { // You're not actually using the time here, but just for demonstration
            // Display the time on the screen
            graphics_drawChar(0, 0, clock_getHourTens12(), FONT_SIZE_5x3, 0xFF0000);
            graphics_drawChar(3, 0, clock_getHourOnes12(), FONT_SIZE_5x3, 0x00FF00);
            if (time.second % 2 == 0) {
                display_manager_setPixel(1,6 , 0xFFFF00); // Draw the colon
                display_manager_setPixel(3,6 , 0xFFFF00); // Draw the colon
            } else {
                display_manager_setPixel(1,6 , 0x000000); // Clear the colon
                display_manager_setPixel(3,6 , 0x000000); // Clear the colon
            }
            graphics_drawChar(7, 0, clock_getMinuteTens(), FONT_SIZE_5x3, 0x0000FF);
            graphics_drawChar(10, 0, clock_getMinuteOnes(), FONT_SIZE_5x3, 0xFFFF00); 

        } else {
            LOGD("Failed to get current time");
        }

        if (http_manager_getCurrentIPStatus() == IP_EVENT_STA_GOT_IP)
        {
            if (http_manager_isRequestInProgress())
            {
                wifiStatusLedState = !wifiStatusLedState;
                uint32_t color = wifiStatusLedState ? GREEN : BLACK;
                display_manager_setPixel(0, 31, color);
            }
            else
            {
                wifiStatusLedState = true;
                display_manager_setPixel(0, 31, GREEN);
            }
        }
        else if (http_manager_getCurrentWifiStatus() == WIFI_EVENT_STA_CONNECTED)
        {
            display_manager_setPixel(0, 31, YELLOW);
        }
        else
        {
            display_manager_setPixel(0, 31, RED);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for 100 ms
    }
}