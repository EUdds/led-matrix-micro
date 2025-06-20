#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_manager.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_APPS 16
#define APP_NAME_MAX_LENGTH 32

typedef enum {
    APP_STATE_STOPPED = 0,
    APP_STATE_STARTING,
    APP_STATE_RUNNING,
    APP_STATE_STOPPING,
    APP_STATE_ERROR
} app_state_t;

typedef struct {
    char name[APP_NAME_MAX_LENGTH];
    bool (*init_function)(void);
    void (*task_function)(void*);
    void (*deinit_function)(void);
    bool active;
    uint8_t priority;
    uint32_t refresh_rate_ms;
    // displayManager_buffer_t* display_buffer;
    TaskHandle_t task_handle;
    size_t stack_size; // Stack size for the task
    app_state_t state;
} app_manager_app_t;

// Core functions
esp_err_t app_manager_init(void);
esp_err_t app_manager_register_app(app_manager_app_t* app);
esp_err_t app_manager_unregister_app(const char* name);

// App lifecycle management
esp_err_t app_manager_start_app(const char* name);
esp_err_t app_manager_stop_app(const char* name);
esp_err_t app_manager_restart_app(const char* name);

// Layout management
esp_err_t app_manager_set_app_position(const char* name, uint32_t x, uint32_t y);
esp_err_t app_manager_set_app_size(const char* name, uint32_t width, uint32_t height);
esp_err_t app_manager_set_app_layer(const char* name, displayManager_layer_E layer);

// Configuration management
esp_err_t app_manager_save_layout(void);
esp_err_t app_manager_load_layout(void);

// Query functions
app_manager_app_t* app_manager_get_app(const char* name);
bool app_manager_is_app_running(const char* name);

void app_manager_task(void* pvParameter);

