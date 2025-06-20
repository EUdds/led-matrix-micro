#include "ota_manger.h"

#include "http_manager.h"
#include "display_manager.h"
#include "utils.h"
#include "telnet_log.h"
#include "app_manager.h"
#include "graphics.h"

#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mbedtls/md5.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "OTA_MANAGER"


static ota_manager_status_E ota_status = OTA_STATUS_IDLE;
static uint8_t currentProgress = 0; // Current progress in the progress bar

static const uint8_t PROGRESS_BAR_ORIENTATION = 0; // 1 for vertical, 0 for horizontal

bool start_ota_server();
bool updater_init(void);
void ota_manager_drawProgressBar(void);

static app_manager_app_t ota_app =
{
    .name = "Updater",
    .init_function = updater_init,
    .task_function = ota_task,
    .deinit_function = NULL, // No specific deinit function
    .active = true,
    .priority = 1,
    .refresh_rate_ms = 1000, // Refresh every second
    .task_handle = NULL,
    .stack_size = 8192,
    .state = APP_STATE_STOPPED,
};

static displayManager_buffer_t* updater_display_buffer = NULL;

bool ota_manager_isUpdateInProgress(void) {
    return (ota_status >= OTA_STATUS_IN_PROGRESS);
}

void ota_manger_colorProgressBar(uint32_t color)
{

    if (PROGRESS_BAR_ORIENTATION == 1)
    {
        uint8_t start_x = (updater_display_buffer->x / 2) - 1;
        uint8_t start_y = updater_display_buffer->y;
        uint8_t end_y = updater_display_buffer->y + updater_display_buffer->height - 1;
        graphics_drawLine(updater_display_buffer, start_x, start_y, start_x, end_y, color); // Draw the vertical line
    }
    else
    {
        uint8_t start_x = updater_display_buffer->x;
        uint8_t start_y = (updater_display_buffer->y / 2) - 1;
        uint8_t end_x = updater_display_buffer->x + updater_display_buffer->width - 1;
        graphics_drawLine(updater_display_buffer, start_x, start_y, end_x, start_y, color); // Draw the horizontal line
    }
}

void ota_mangager_drawProgress(void)
{
    if (PROGRESS_BAR_ORIENTATION == 1) // Vertical progress bar
    {
        uint8_t start_x = (updater_display_buffer->x / 2) - 1;
        uint8_t start_y = updater_display_buffer->y + (updater_display_buffer->height / 2) - 1;
        uint8_t end_y = updater_display_buffer->y + (updater_display_buffer->height * currentProgress / 100);
        graphics_drawLine(updater_display_buffer, start_x, start_y, start_x, end_y, YELLOW); // Draw the vertical line
    }
    else // Horizontal progress bar
    {
        uint8_t start_x = 0;
        uint8_t start_y = updater_display_buffer->y + (updater_display_buffer->height / 2) - 1;
        uint8_t end_x = updater_display_buffer->x + (updater_display_buffer->width * currentProgress / 100);
        graphics_drawLine(updater_display_buffer, start_x, start_y, end_x, start_y, YELLOW); // Draw the horizontal line
    }
}

void updater_drawUpdater(void)
{
    ota_manager_drawProgressBar();
}

void ota_manager_drawProgressBar(void) {
    switch (ota_status)
    {
        case OTA_STATUS_IDLE:
        {
            updater_display_buffer->active = false;
            break;
        }
        case OTA_STATUS_STARTING:
        {
            updater_display_buffer->active = true;
            graphics_drawRectangle(updater_display_buffer, 
                                    updater_display_buffer->x, 
                                    updater_display_buffer->y, 
                                    updater_display_buffer->width, 
                                    updater_display_buffer->height, 
                                    BLACK); // Clear the rectangle
            currentProgress = 0;
            break;
        }
        case OTA_STATUS_IN_PROGRESS:
        {
            updater_display_buffer->active = true;
            ota_mangager_drawProgress();
            break;
        }
        case OTA_STATUS_SUCCESS:
        {
            updater_display_buffer->active = true;
            ota_manger_colorProgressBar(GREEN);
            break;
        }
        case OTA_STATUS_FAILED:
        {
            updater_display_buffer->active = true;
            ota_manger_colorProgressBar(RED);
            break;
        }
    }
}

esp_err_t ota_post_handler(httpd_req_t *req) {
    esp_ota_handle_t ota_handle = 0;
    ota_status = OTA_STATUS_STARTING;
    updater_drawUpdater();
    // Log some information about the request
    LOGI("Received OTA request: %s", req->uri);
    LOGI("Content-Length: %d", req->content_len);
    if (req->content_len <= 0) {
        LOGE("Invalid content length: %d", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }

    char md5_str[33] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-MD5", md5_str, sizeof(md5_str)) <= 0) {
        LOGE("MD5 header not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "MD5 header not found");
        return ESP_FAIL;
    }
    else
    {
        LOGI("MD5 Header: %s", md5_str);
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        LOGE("No OTA partition found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    ota_status = OTA_STATUS_IN_PROGRESS;
    LOGI("Writing to partition: %s", update_partition->label);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        LOGE("esp_ota_begin failed");
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
        updater_drawUpdater();
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
        LOGE("Error in receiving OTA file");
        esp_ota_end(ota_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA receive error");
        ota_status = OTA_STATUS_FAILED;
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        LOGE("esp_ota_end failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        ota_status = OTA_STATUS_FAILED;
        updater_drawUpdater();
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        LOGE("esp_ota_set_boot_partition failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
        ota_status = OTA_STATUS_FAILED;
        updater_drawUpdater();
        return ESP_FAIL;
    }

    LOGI("OTA update successful, restarting...");
    httpd_resp_sendstr(req, "OK - Rebooting");
    ota_status = OTA_STATUS_SUCCESS;
    updater_drawUpdater();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

bool start_ota_server() {
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

    if (server == NULL) {
        LOGE("Failed to start OTA server");
        return false;
    }
    return true;
}

bool updater_init(void)
{
    updater_display_buffer = display_manager_create_buffer("Updater",
                                                              DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                                              0, 0,
                                                              DISPLAY_MANAGER_LAYER_SYSTEM);
    if (updater_display_buffer == NULL) {
        LOGE("Failed to create updater display buffer");
        return false; // Buffer creation failed
    }

    if (false == start_ota_server())
    {
        LOGE("Failed to start OTA server");
        return false; // Server failed to start
    }

    return true;
}

void ota_task(void* pvParameter) {
    LOGI("OTA Task started");
    ota_status = OTA_STATUS_IDLE;

    while (1) {
        // Keep the task running
        if (ota_manager_isUpdateInProgress())
        {
            updater_drawUpdater();
            vTaskDelay(pdMS_TO_TICKS(100)); // Update the progress bar every 100 ms
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(2500)); // Update the progress bar every 100 ms
        }
    }
}

esp_err_t updater_register(void)
{
    if (app_manager_register_app(&ota_app) != ESP_OK) {
        LOGE("Failed to register OTA app");
        return ESP_FAIL;
    }
    return ESP_OK;
}