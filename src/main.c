#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hardware.h"
#include "freertos/queue.h" 
#include "neopixel_driver.h"
#include "display_manager.h"
#include "utils.h"

#define LED_PIN GPIO_NUM_2  // Built-in LED on most ESP32 dev boards

void blinky_task(void *pvParameter) {
    // Configure the LED pin as output
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE, // No interrupt
        .mode = GPIO_MODE_OUTPUT,       // Set as output mode
        .pin_bit_mask = (1ULL << LED_PIN), // Bit mask of the pin
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // No pull-down
        .pull_up_en = GPIO_PULLUP_DISABLE // No pull-up
    };
    gpio_config(&io_conf); // Apply the configuration
    while(1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    xTaskCreate(&blinky_task, "blinky_task", 2048, NULL, 5, NULL);
    xTaskCreate(&neopixel_task, "neopixel_task", 4096, NULL, 5, NULL); 
    xTaskCreate(&hardware_task, "hardware_task", 4096, NULL, 5, NULL); // Create the hardware task
    xTaskCreate(&display_manager_task, "display_manager_task", 4096, NULL, 5, NULL); // Create the display manager task
    int headCol = 0;
    int headRow = 0;
    int prevHeadCol = 0;
    int prevHeadRow = 0;
    bool colDirection = true;
    // display_manager_setPixel(0, 0, RED);
    // display_manager_setPixel(0, 1, GREEN);
    // display_manager_setPixel(0, 2, BLUE);
    while(1)
    {

        display_manager_setPixel(headRow, headCol, RED);
        display_manager_setPixel(prevHeadRow, prevHeadCol, BLACK);
        prevHeadCol = headCol;
        prevHeadRow = headRow;
        headCol += colDirection ? 1 : -1;
        if (headCol >= NEOPIXEL_NUM_COLS) {
            headCol = NEOPIXEL_NUM_COLS;
            colDirection = false;
            headRow++;
        }
        if (headCol < 0) {
            headCol = 0;
            colDirection = true;
            headRow++;
        }

        if (headRow >= NEOPIXEL_NUM_ROWS) {
            headRow = 0;
            headCol = 0;
            colDirection = true;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for 100 ms

    }
    

}