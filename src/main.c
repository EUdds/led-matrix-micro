#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hardware.h"
#include "freertos/queue.h" 
#include "neopixel_driver.h"
#include "display_manager.h"
#include "utils.h"
#include "graphics.h"
#include "clock.h"
#include "http_manager.h"
#include "ota_manger.h"

#define LED_PIN GPIO_NUM_2  // Built-in LED on most ESP32 dev boards

void blinky_task(void *pvParameter) {
    // Configure the LED pin as output
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
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
    xTaskCreate(&blinky_task, "blinky_task", 1024, NULL, 5, NULL);
    xTaskCreate(&neopixel_task, "neopixel_task", 4096, NULL, 5, NULL); 
    xTaskCreate(&hardware_task, "hardware_task", 4096, NULL, 5, NULL); // Create the hardware task
    xTaskCreate(&display_manager_task, "display_manager_task", 4096, NULL, 5, NULL); // Create the display manager task
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL); // Create the HTTP task
    xTaskCreate(&http_task, "http_task", 8192, NULL, 5, NULL); // Create the HTTP task
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL); // Create the OTA task
    
    while(1)
    {
        // for (int x=0; x < NEOPIXEL_NUM_COLS - 3; x++)
        // {
        //     for (int y=0; y < NEOPIXEL_NUM_ROWS - 5; y++)
        //     {
        //         for (int i=0; i < 11; i++)
        //         {
        //             char c = (char)('0' + i); // Convert to character '0' to '9'
        //             graphics_drawChar(x, y, c, FONT_SIZE_5x3, 0xFF0000); // Draw the character in red
        //             vTaskDelay(pdMS_TO_TICKS(250)); // Wait for 50 ms
        //             graphics_drawChar(x, y, c, FONT_SIZE_5x3, 0x000000); // Clear the character

        //         }
        //         vTaskDelay(pdMS_TO_TICKS(150)); // Wait for 1 second
        //     }
        // }


        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for 100 ms

    }
    

}