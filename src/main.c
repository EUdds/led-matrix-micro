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
#include "telnet_log.h"
#include "genealogy.h"

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
    // if (genealogy_init() != ESP_OK) {
    //     LOGE("Failed to initialize genealogy");
    //     while (1) {
    //         vTaskDelay(pdMS_TO_TICKS(1000)); // Wait indefinitely
    //     }
    // } else {
    //     LOGI("Genealogy initialized successfully");
    // }

    // vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second before starting tasks
    esp_err_t err = display_manager_init();
    ESP_ERROR_CHECK(err); // Initialize the display manager

    ESP_ERROR_CHECK(app_manager_init());

    ESP_ERROR_CHECK(clock_app_register());
    ESP_ERROR_CHECK(updater_register());

    xTaskCreate(&blinky_task, "blinky_task", 1024, NULL, 5, NULL);
    xTaskCreate(&neopixel_task, "neopixel_task", 8192, NULL, 5, NULL); 
    xTaskCreate(&hardware_task, "hardware_task", 4096, NULL, 5, NULL); // Create the hardware task
    xTaskCreate(&display_manager_task, "display_manager_task", 4096, NULL, 5, NULL); // Create the display manager task
    xTaskCreate(&http_task, "http_task", 8192, NULL, 5, NULL); // Create the HTTP task
    // xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL); // Create the OTA task
    xTaskCreate(&telnet_log_task, "telnet_log_task", 8192, NULL, 5, NULL); // Create the Telnet log task
    
    xTaskCreate(&app_manager_task, "app_manager_task", 8192, NULL, 5, NULL); // Create the app manager task
    
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for 100 ms

    }
    

}