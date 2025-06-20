#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

esp_err_t nvs_utils_init(const char *namespace);
esp_err_t nvs_utils_setu8(const char *key, uint8_t value);
esp_err_t nvs_utils_getu8(const char *key, uint8_t *value);
esp_err_t nvs_utils_setString(const char *key, const char *value);
esp_err_t nvs_utils_getString(const char *key, char *value, size_t max_len);
esp_err_t nvs_utils_setFloat(const char *key, float value);
esp_err_t nvs_utils_getFloat(const char *key, float *value);
bool nvs_utils_is_initialized(void);
