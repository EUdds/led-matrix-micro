#include "ota_manger.h"

#include "http_manager.h"
#include "display_manager.h"
#include "utils.h"

#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "OTA_MANAGER"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)


static ota_manager_status_E ota_status = OTA_STATUS_IDLE;
static uint8_t currentProgress = 0; // Current progress in the progress bar

static const uint8_t NUM_LEDS = 7; // Number of LEDs in the progress bar
static const uint8_t STARTING_X = 31;
static const uint8_t STARTING_Y = 1; // Starting position for the progress bar
static const uint8_t PROGRESS_BAR_ORIENTATION = 1; // 1 for vertical, 0 for horizontal

bool ota_manager_isUpdateInProgress(void) {
    return (ota_status == OTA_STATUS_IN_PROGRESS);
}

void ota_manger_colorProgressBar(uint32_t color)
{
    if (PROGRESS_BAR_ORIENTATION == 1)
    {
        int cur_x = STARTING_X;
        int cur_y = STARTING_Y;
        for (int i = 0; i < NUM_LEDS; i++)
        {
            display_manager_setPixel(cur_y + i, cur_x, color);
        }
    }
}

void ota_mangager_drawProgress(void)
{
    int cur_x = STARTING_X;
    int cur_y = STARTING_Y;
    int leds_to_inc = currentProgress * NUM_LEDS / 100; // Calculate how many LEDs to light up
    if (PROGRESS_BAR_ORIENTATION == 1)
    {
        for (int i = 0; i < leds_to_inc; i++)
        {
            display_manager_setPixel(cur_y+i, cur_x, YELLOW); // Draw the LED
        }
    }
    else
    {
        int cur_x = STARTING_X;
        int cur_y = STARTING_Y;
        for (int i = 0; i < leds_to_inc; i++)
        {
            display_manager_setPixel(cur_y, cur_x + i, YELLOW); // Draw the LED
        }
    }
}



void ota_manager_drawProgressBar(void) {
    switch (ota_status)
    {
        case OTA_STATUS_IDLE:
        case OTA_STATUS_STARTING:
        {
            currentProgress = 0;
            ota_manger_colorProgressBar(BLACK);
            break;
        }
        case OTA_STATUS_IN_PROGRESS:
        {
            ota_mangager_drawProgress();
            break;
        }
        case OTA_STATUS_SUCCESS:
        {
            ota_manger_colorProgressBar(GREEN);
            break;
        }
        case OTA_STATUS_FAILED:
        {
            ota_manger_colorProgressBar(RED);
            break;
        }
    }
}

esp_err_t ota_post_handler(httpd_req_t *req) {
    esp_ota_handle_t ota_handle = 0;
    ota_status = OTA_STATUS_STARTING;
    // Log some information about the request
    LOGI("Received OTA request: %s", req->uri);
    LOGI("Content-Length: %d", req->content_len);
    if (req->content_len <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No OTA partition found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    ota_status = OTA_STATUS_IN_PROGRESS;
    ESP_LOGI(TAG, "Writing to partition: %s", update_partition->label);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    char ota_buff[1024];
    int received;
    int rx_counter = 0;
    while ((received = httpd_req_recv(req, ota_buff, sizeof(ota_buff))) > 0) {
        rx_counter += received;
        currentProgress = (rx_counter * 100) / req->content_len;
        LOGI("Received %d/%d bytes (%d%%)", rx_counter, req->content_len, currentProgress);
        // Print the first few bytes for debugging
        err = esp_ota_write(ota_handle, (const void *)ota_buff, received);
        switch (err)
        {
            case ESP_OK:
            {
                break;
            }

            case ESP_ERR_OTA_VALIDATE_FAILED:
            {
                LOGE("esp_ota_write_failed");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Write Failed");
                ota_status = OTA_STATUS_FAILED;
                break;
            }

            case ESP_ERR_INVALID_SIZE:
                LOGE("esp_ota_invalid_size");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Size");
                ota_status = OTA_STATUS_FAILED;
                break;
        }
        if (err != ESP_OK) {
            esp_ota_end(ota_handle);
            return ESP_FAIL;
        }
    }

    if (received < 0) {
        ESP_LOGE(TAG, "Error in receiving OTA file");
        esp_ota_end(ota_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA receive error");
        ota_status = OTA_STATUS_FAILED;
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        ota_status = OTA_STATUS_FAILED;
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
        ota_status = OTA_STATUS_FAILED;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA update successful, restarting...");
    httpd_resp_sendstr(req, "OK - Rebooting");
    ota_status = OTA_STATUS_SUCCESS;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

httpd_handle_t start_ota_server() {
    LOGI("Waiting for Wifi...");
    while (!http_manager_readyForDependencies()) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
    LOGI("Starting OTA server...");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ota_post_uri = {
            .uri = "/update",
            .method = HTTP_POST,
            .handler = ota_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &ota_post_uri);
    }
    return server;
}


void ota_task(void* pvParameter) {
    LOGI("OTA Task started");
    httpd_handle_t ota_server = start_ota_server();
    ota_status = OTA_STATUS_IDLE;

    if (ota_server == NULL) {
        LOGE("Failed to start OTA server");
        vTaskDelete(NULL);
    }

    while (1) {
        // Keep the task running
        ota_manager_drawProgressBar();
        if (ota_manager_isUpdateInProgress())
        {
            vTaskDelay(pdMS_TO_TICKS(100)); // Update the progress bar every 100 ms
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(2500)); // Update the progress bar every 100 ms
        }
    }
}