#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_SSID_LENGTH 32
#define MAX_PASS_LENGTH 64
#define MAX_SERIAL_LENGTH 32

typedef struct {
    float brightness;
    char wifi_ssid[MAX_SSID_LENGTH];
    char wifi_password[MAX_PASS_LENGTH];
    char serial[MAX_SERIAL_LENGTH];
} genealogy_t;

#define GENEALOGY_NAMESPACE "genealogy"

// Storage keys
#define KEY_BRIGHTNESS "brightness"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"
#define KEY_SERIAL "serial"

esp_err_t genealogy_init(void);

// Brightness functions
esp_err_t genealogy_set_brightness(float brightness);
esp_err_t genealogy_get_brightness(float *brightness);

// WiFi credential functions
esp_err_t genealogy_set_wifi_credentials(const char *ssid, const char *password);
esp_err_t genealogy_get_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len);

// Serial number functions
esp_err_t genealogy_set_serial(const char *serial);
esp_err_t genealogy_get_serial(char *serial, size_t serial_len);
