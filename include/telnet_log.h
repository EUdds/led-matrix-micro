#ifndef TELNET_LOG_H
#define TELNET_LOG_H

#include <stdbool.h>
#include "esp_log.h"

void telnet_log_init(void);
void telnet_log_write(const char* fmt, ...);
bool telnet_log_is_client_connected(void);
void telnet_log_task(void* pvParameter);

// Replace existing ESP logging macros
#undef LOGI
#undef LOGE
#undef LOGD
#undef LOGW

#define LOGI(...) do { ESP_LOGI(TAG, __VA_ARGS__); if (telnet_log_is_client_connected()) telnet_log_write("I (%s): ", TAG); telnet_log_write(__VA_ARGS__); telnet_log_write("\n\r"); } while(0)
#define LOGE(...) do { ESP_LOGE(TAG, __VA_ARGS__); if (telnet_log_is_client_connected()) telnet_log_write("E (%s): ", TAG); telnet_log_write(__VA_ARGS__); telnet_log_write("\n\r"); } while(0)
#define LOGD(...) do { ESP_LOGD(TAG, __VA_ARGS__); if (telnet_log_is_client_connected()) telnet_log_write("D (%s): ", TAG); telnet_log_write(__VA_ARGS__); telnet_log_write("\n\r"); } while(0)
#define LOGW(...) do { ESP_LOGW(TAG, __VA_ARGS__); if (telnet_log_is_client_connected()) telnet_log_write("W (%s): ", TAG); telnet_log_write(__VA_ARGS__); telnet_log_write("\n\r"); } while(0)

#endif // TELNET_LOG_H