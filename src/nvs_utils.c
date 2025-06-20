#include "nvs_flash.h"
#include "nvs.h"
#include "telnet_log.h"

#include <string.h>

#define TAG "NVS"

static nvs_handle_t local_nvs_handle = 0; // Handle for the NVS namespace

esp_err_t nvs_utils_init(const char *namespace) 
{

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        // Retry initialization after erasing
        err = nvs_flash_erase();
        if (err != ESP_OK) 
        {
            LOGE("Failed to erase NVS flash: %s", esp_err_to_name(err));
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) 
    {
        LOGE("Failed to initialize NVS flash: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_open(namespace, NVS_READWRITE, &local_nvs_handle);
    if (err != ESP_OK) 
    {
        LOGE("Failed to open NVS namespace '%s': %s", namespace, esp_err_to_name(err));
        return err;
    }

    LOGI("NVS namespace '%s' opened successfully", namespace);
    return ESP_OK;
}

esp_err_t nvs_utils_setu8(const char *key, uint8_t value) 
{
    uint32_t data __attribute__((aligned(4)));
    memcpy(&data, &value, sizeof(uint8_t));

    esp_err_t err = nvs_set_u8(local_nvs_handle, key, value);
    if (err != ESP_OK) 
    {
        LOGE("Failed to set u8 for key '%s': %s", key, esp_err_to_name(err));
    }
    
    err = nvs_commit(local_nvs_handle);
    if (err != ESP_OK) {
        LOGE("Failed to commit NVS changes for key '%s': %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_utils_getu8(const char *key, uint8_t *value) 
{
    esp_err_t err = nvs_get_u8(local_nvs_handle, key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        LOGW("Key '%s' not found", key);
        return ESP_OK; // Not an error, just means the key doesn't exist
    } 
    else if (err != ESP_OK) 
    {
        LOGE("Failed to get u8 for key '%s': %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_utils_setString(const char *key, const char *value) 
{
    esp_err_t err = nvs_set_str(local_nvs_handle, key, value);
    if (err != ESP_OK) 
    {
        LOGE("Failed to set string for key '%s': %s", key, esp_err_to_name(err));
    }
    
    err = nvs_commit(local_nvs_handle);
    if (err != ESP_OK) {
        LOGE("Failed to commit NVS changes for key '%s': %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_utils_getString(const char *key, char *value, size_t max_len) 
{
    esp_err_t err = nvs_get_str(local_nvs_handle, key, value, &max_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        LOGW("Key '%s' not found", key);
        return ESP_OK; // Not an error, just means the key doesn't exist
    } 
    else if (err != ESP_OK) 
    {
        LOGE("Failed to get string for key '%s': %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_utils_setFloat(const char *key, float value) {
    if (local_nvs_handle == 0) {
        LOGE("NVS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create aligned storage
    static union {
        float f;
        uint32_t u;
    } data __attribute__((aligned(4)));
    
    data.f = value;
    
    esp_err_t err = nvs_set_u32(local_nvs_handle, key, data.u);
    if (err != ESP_OK) {
        LOGE("Failed to set float for key '%s': %s", key, esp_err_to_name(err));
        return err;
    }

    err = nvs_commit(local_nvs_handle);
    if (err != ESP_OK) {
        LOGE("Failed to commit NVS changes for key '%s': %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_utils_getFloat(const char *key, float *value) {
    if (local_nvs_handle == 0 || value == NULL) {
        LOGE("Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    static union {
        float f;
        uint32_t u;
    } data __attribute__((aligned(4)));

    esp_err_t err = nvs_get_u32(local_nvs_handle, key, &data.u);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        LOGW("Key '%s' not found", key);
        return ESP_OK;
    } else if (err != ESP_OK) {
        LOGE("Failed to get float for key '%s': %s", key, esp_err_to_name(err));
        return err;
    }
    
    *value = data.f;
    return ESP_OK;
}

bool nvs_utils_is_initialized(void)
{
    return local_nvs_handle != 0;
}