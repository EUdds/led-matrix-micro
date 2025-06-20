#include "app_manager.h"

#include "telnet_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include <string.h>

#define TAG "APP_MANAGER"

typedef struct {
    app_manager_app_t* apps[MAX_APPS];
    uint32_t num_apps;
    bool initialized;
} app_manager_ctx_t;

static app_manager_ctx_t am_ctx = {0};

esp_err_t app_manager_init(void)
{
    if (am_ctx.initialized) {
        return ESP_OK;
    }

    am_ctx.initialized = true;
    ESP_LOGI(TAG, "App manager initialized");
    return ESP_OK;
}

esp_err_t app_manager_register_app(app_manager_app_t* app)
{
    if (!am_ctx.initialized || !app || !app->name[0]) {
        return ESP_ERR_INVALID_STATE;
    }

    if (am_ctx.num_apps >= MAX_APPS) {
        ESP_LOGE(TAG, "Maximum number of apps reached");
        return ESP_ERR_NO_MEM;
    }

    // Check for duplicate app names
    for (int i = 0; i < am_ctx.num_apps; i++) {
        if (strcmp(am_ctx.apps[i]->name, app->name) == 0) {
            ESP_LOGE(TAG, "App '%s' already registered", app->name);
            return ESP_ERR_INVALID_STATE;
        }
    }

    am_ctx.apps[am_ctx.num_apps++] = app;
    ESP_LOGI(TAG, "Registered app '%s'", app->name);

    return ESP_OK;
}

app_manager_app_t* app_manager_get_app(const char* name)
{
    if (!name || !am_ctx.initialized) {
        return NULL;
    }

    for (int i = 0; i < am_ctx.num_apps; i++) {
        if (strcmp(am_ctx.apps[i]->name, name) == 0) {
            return am_ctx.apps[i];
        }
    }

    ESP_LOGE(TAG, "App '%s' not found", name);
    return NULL;
}

esp_err_t app_manager_start_app(const char* name)
{
    app_manager_app_t* app = app_manager_get_app(name);
    if (!app) {
        return ESP_ERR_NOT_FOUND;
    }

    if (app->state == APP_STATE_RUNNING) {
        return ESP_OK;
    }

    app->state = APP_STATE_STARTING;

    // Initialize the app
    if (app->init_function && !app->init_function()) {
        LOGE("Failed to initialize app '%s'", app->name);
        app->state = APP_STATE_ERROR;
        return ESP_FAIL;
    }
    LOGI("App '%s' initialized successfully", app->name);

    TaskHandle_t task_handle;
    // Create the app's task
    BaseType_t result = xTaskCreate(app->task_function, app->name, app->stack_size, NULL, app->priority, &task_handle);


    if (result != pdPASS) {
        LOGE("Failed to create task for app '%s'", app->name);
        app->state = APP_STATE_ERROR;
        return ESP_ERR_NO_MEM;
    }

    LOGI("Created task for app '%s' with handle %p", app->name, task_handle);

    app->state = APP_STATE_RUNNING;
    app->active = true;
    LOGI("Started app '%s'", app->name);

    return ESP_OK;
}

esp_err_t app_manager_stop_app(const char* name)
{
    // Stop the application with the given name
    // This can include calling the app's deinit function and stopping its task
    if (name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // For simplicity, we assume stopping the app is always successful
    return ESP_OK;
}

void app_manager_task(void* pvParameter)
{
    
    // This task can manage the lifecycle of registered applications
    // It can periodically check the status of apps and perform necessary actions
    for (int i = 0; i < am_ctx.num_apps; i++) {
        app_manager_app_t* app = am_ctx.apps[i];
        if (app->active && app->state != APP_STATE_RUNNING) {
            esp_err_t err = app_manager_start_app(app->name);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to start app '%s': %s", app->name, esp_err_to_name(err));
            }
        }
    }

    while (1) {
        // For simplicity, we just delay indefinitely
        vTaskDelay(portMAX_DELAY);
    }
}