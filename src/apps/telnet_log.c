#include "telnet_log.h"

#include "http_manager.h"

#include "lwip/sockets.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdarg.h>
#include <string.h>



static const uint8_t TELNET_PORT = 23;
static const uint8_t MAX_CLIENTS = 1;
#define BUFFER_SIZE 256


static int server_socket = -1;
static int client_socket = -1;

void telnet_log_task(void* pvParameter)
{
    while (!http_manager_readyForDependencies()) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buf[BUFFER_SIZE];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        ESP_LOGE("TELNET_LOG", "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    server_addr.sin_port = htons(TELNET_PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE("TELNET_LOG", "Failed to bind socket");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }

    listen(server_socket, MAX_CLIENTS);
    ESP_LOGI("TELNET_LOG", "Telnet server listening on port %d", TELNET_PORT);

    while(1)
    {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            ESP_LOGE("TELNET_LOG", "Failed to accept client connection");
            continue;
        }

        ESP_LOGI("TELNET_LOG", "Client connected");

        while (1) {
            int len = recv(client_socket, buf, sizeof(buf) - 1, 0);
            if (len <= 0) {
                ESP_LOGI("TELNET_LOG", "Client disconnected");
                close(client_socket);
                break;
            }
            buf[len] = '\0'; // Null-terminate the received string
            ESP_LOGI("TELNET_LOG", "Received: %s", buf);
        }
    }
}

void telnet_log_write(const char* fmt, ...)
{
    if (client_socket < 0) {
        return;
    }

    char buf[BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send(client_socket, buf, strlen(buf), 0);
}

bool telnet_log_is_client_connected(void)
{
    return (client_socket >= 0);
}