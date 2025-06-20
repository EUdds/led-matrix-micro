#include "genealogy.h"
#include "nvs_utils.h"
#include "telnet_log.h"

#include <string.h>

#define TAG "GENEALOGY"

static genealogy_t genealogy = {
    .brightness = 0.5f, // Default brightness
    .wifi_ssid = "",
    .wifi_password = "",
    .serial = ""
};

esp_err_t genealogy_init(void)
{
    LOGD("Initializing genealogy storage");
    esp_err_t err = nvs_utils_init(GENEALOGY_NAMESPACE);
    if (err != ESP_OK) return err;
    LOGD("NVS initialized successfully");

    // Load all values from NVS
    err = nvs_utils_getFloat(KEY_BRIGHTNESS, &genealogy.brightness);
    if (err != ESP_OK) genealogy.brightness = 0.5f; // Default value

    err = nvs_utils_getString(KEY_WIFI_SSID, genealogy.wifi_ssid, MAX_SSID_LENGTH);
    if (err != ESP_OK) genealogy.wifi_ssid[0] = '\0';

    err = nvs_utils_getString(KEY_WIFI_PASS, genealogy.wifi_password, MAX_PASS_LENGTH);
    if (err != ESP_OK) genealogy.wifi_password[0] = '\0';

    err = nvs_utils_getString(KEY_SERIAL, genealogy.serial, MAX_SERIAL_LENGTH);
    if (err != ESP_OK) genealogy.serial[0] = '\0';
    LOGD("Genealogy initialized: brightness=%.2f, wifi_ssid='%s', wifi_password='%s', serial='%s'",
         genealogy.brightness, genealogy.wifi_ssid, genealogy.wifi_password, genealogy.serial);

    return ESP_OK;
}

esp_err_t genealogy_set_brightness(float brightness) {
    esp_err_t err = nvs_utils_setFloat(KEY_BRIGHTNESS, brightness);
    if (err == ESP_OK) {
        genealogy.brightness = brightness;
    }
    return err;
}

esp_err_t genealogy_get_brightness(float *brightness) {
    *brightness = genealogy.brightness;
    return ESP_OK;
}

esp_err_t genealogy_set_wifi_credentials(const char *ssid, const char *password) {
    esp_err_t err = nvs_utils_setString(KEY_WIFI_SSID, ssid);
    if (err != ESP_OK) return err;
    
    err = nvs_utils_setString(KEY_WIFI_PASS, password);
    if (err == ESP_OK) {
        strncpy(genealogy.wifi_ssid, ssid, MAX_SSID_LENGTH - 1);
        genealogy.wifi_ssid[MAX_SSID_LENGTH - 1] = '\0';
        strncpy(genealogy.wifi_password, password, MAX_PASS_LENGTH - 1);
        genealogy.wifi_password[MAX_PASS_LENGTH - 1] = '\0';
    }
    return err;
}

esp_err_t genealogy_get_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len) {
    strncpy(ssid, genealogy.wifi_ssid, ssid_len - 1);
    ssid[ssid_len - 1] = '\0';
    strncpy(password, genealogy.wifi_password, pass_len - 1);
    password[pass_len - 1] = '\0';
    return ESP_OK;
}

esp_err_t genealogy_set_serial(const char *serial) {
    esp_err_t err = nvs_utils_setString(KEY_SERIAL, serial);
    if (err == ESP_OK) {
        strncpy(genealogy.serial, serial, MAX_SERIAL_LENGTH - 1);
        genealogy.serial[MAX_SERIAL_LENGTH - 1] = '\0';
    }
    return err;
}

esp_err_t genealogy_get_serial(char *serial, size_t serial_len) {
    strncpy(serial, genealogy.serial, serial_len - 1);
    serial[serial_len - 1] = '\0';
    return ESP_OK;
}