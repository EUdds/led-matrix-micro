#include "http_manager.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define TAG "HTTP_MANAGER"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)


static EventGroupHandle_t s_wifi_event_group = NULL;
static QueueHandle_t s_http_event_queue = NULL;
static const int GOT_IP_BIT = BIT0;
static char response_buffer[MAX_RESPONSE_LENGTH] = {0};
static size_t response_length = 0;
static int32_t currentWifiStatus = 0;
static int32_t currentIPStatus = 65535;
static bool  requestInProgress = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        currentWifiStatus = event_id;
    }
    else if (event_base == IP_EVENT)
    {
        currentIPStatus = event_id;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Reconnecting...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, GOT_IP_BIT);
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP connected");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "Header: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            // chunked or not, just print the data as it arrives
            if (response_length + evt->data_len < MAX_RESPONSE_LENGTH) {
                memcpy(response_buffer + response_length, evt->data, evt->data_len);
                response_length += evt->data_len;
            } else {
                ESP_LOGE(TAG, "Response buffer overflow");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            response_buffer[response_length] = '\0'; // Null-terminate the string
            ESP_LOGI(TAG, "\n--- HTTP finished ---");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP disconnected");
            break;
        default:
            break;
    }
    return ESP_OK;
}

static bool http_manager_doGetRequest(const char* url, char* resp, size_t* resp_len)
{
    if (currentIPStatus != IP_EVENT_STA_GOT_IP) {
        LOGE("Wi-Fi not connected, cannot perform HTTP GET request");
        return false;
    }

    memset(response_buffer, 0, sizeof(response_buffer));
    response_length = 0;
    esp_http_client_config_t config = {
        .url            = url,
        .event_handler  = http_event_handler,
        // increase timeout if needed:
        .timeout_ms     = 10000,
        .buffer_size = 2048,
        .method = HTTP_METHOD_GET,
        .user_data = response_buffer, // Pass response buffer to the event handler
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (!client)
    {
        LOGE("Failed to initialize HTTP client");
        return false;
    }

    // make sure to close the connection after each request
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "X-HTTP-Protocol", "HTTP/1.0");

    LOGI("Performing GET request to %s", url);
    requestInProgress = true;
    esp_err_t err = esp_http_client_perform(client);
    requestInProgress = false;
    bool success = false;

    if (err == ESP_OK) {
        LOGI("HTTP GET Status = %d, content_length = %lld", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
        if (resp && resp_len) {
            memcpy(resp, response_buffer, response_length);
            *resp_len = response_length;
            success = true;
        }
    } else {
        LOGE("HTTP GET failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return success;
}

void http_manager_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. Create default Wi‑Fi station
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config =
    {
        .sta =
        {
            .ssid = "Pretty Fly for a Wi-Fi",
            .password = "Getyour0wnwifi",
        }
    };
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        LOGE("Failed to set Wi-Fi mode: %s", esp_err_to_name(ret));
        return;
    }
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        LOGE("Failed to start Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }
    xEventGroupWaitBits(s_wifi_event_group, GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    LOGI("Wi-Fi initialized and connected");

    s_http_event_queue = xQueueCreate(10, sizeof(esp_http_client_event_t*));
    if (s_http_event_queue == NULL) {
        LOGE("Failed to create HTTP event queue");
        return;
    }
    LOGI("HTTP event queue created successfully");
}

int32_t http_manager_getCurrentWifiStatus()
{
    return currentWifiStatus;
}

int32_t http_manager_getCurrentIPStatus()
{
    return currentIPStatus;
}

bool http_manager_isRequestInProgress()
{
    return requestInProgress;
}

bool http_manager_readyForDependencies(void)
{
    if (s_http_event_queue == NULL) {
        LOGE("HTTP event queue not initialized");
        return false;
    }
    if (s_wifi_event_group == NULL) {
        LOGE("Wi-Fi event group not initialized");
        return false;
    }
    if (currentIPStatus != IP_EVENT_STA_GOT_IP) {
        LOGE("Wi-Fi not connected, cannot perform HTTP requests");
        return false;
    }
    return true;
}

bool http_manager_httpGet(const char* url, char* resp, size_t* resp_len)
{
    if (!url || !resp || !resp_len)
    {
        LOGE("Invalid arguments for HTTP GET request");
        return false;
    }

    http_manager_requestQueueItem_t* request = pvPortMalloc(sizeof(http_manager_requestQueueItem_t));
    if (!request)
    {
        LOGE("Failed to allocate memory for HTTP request");
        return false;
    }

    request->response = resp;
    request->response_len = resp_len;
    request->done = xSemaphoreCreateBinary();
    request->success = false;
    strncpy(request->url, url, sizeof(request->url) - 1);
    request->url[sizeof(request->url) - 1] = '\0'; // Ensure null-termination

    LOGI("Adding HTTP request to queue: %s", request->url); 
    if (xQueueSend(s_http_event_queue, &request, portMAX_DELAY) != pdTRUE)
    {
        LOGE("Failed to send HTTP request to queue");
        vSemaphoreDelete(request->done);
        return false;
    }

    if (xSemaphoreTake(request->done, pdMS_TO_TICKS(30000)) != pdTRUE)
    {
        LOGE("HTTP request timed out");
        vSemaphoreDelete(request->done);
        return false;
    }

    vSemaphoreDelete(request->done);
    return request->success;
}

void http_task(void* pvParameter)
{
    http_manager_init();
    http_manager_requestQueueItem_t* currentAction;
    LOGI("Starting to listen for HTTP requests...");
    while (1)
    {
        if (xQueueReceive(s_http_event_queue, &currentAction, portMAX_DELAY) == pdTRUE)
        {
            LOGI("Processing HTTP request: %s", currentAction->url);
            currentAction->success = http_manager_doGetRequest(
                currentAction->url,
                currentAction->response, 
                currentAction->response_len);
            xSemaphoreGive(currentAction->done);
        }


    }
}