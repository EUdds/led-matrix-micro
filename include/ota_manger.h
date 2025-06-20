#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_ota_ops.h"

typedef enum
{
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_STARTING,
    OTA_STATUS_IN_PROGRESS,
    OTA_STATUS_SUCCESS,
    OTA_STATUS_FAILED,
} ota_manager_status_E;

void ota_manager_init(void);
void ota_task(void* pvParameter);
bool ota_manager_isUpdateInProgress(void);
ota_manager_status_E ota_manager_getStatus(void);
esp_err_t updater_register(void);