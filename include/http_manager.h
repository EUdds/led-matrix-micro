#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_http_client.h"
#include "esp_event.h"

#define MAX_RESPONSE_LENGTH 2048
#define MAX_URL_LENGTH 256

typedef struct
{
    char url[MAX_URL_LENGTH];
    char* response;
    size_t* response_len;
    SemaphoreHandle_t done;
    bool success;
    esp_http_client_method_t method;
} http_manager_requestQueueItem_t;


void http_manager_init(void);
bool http_manager_httpGet(const char* url, char* resp, size_t* resp_len);
int32_t http_manager_getCurrentWifiStatus();
int32_t http_manager_getCurrentIPStatus();
bool http_manager_isRequestInProgress();
void http_task(void* pvParameter);
bool http_manager_readyForDependencies(void);
