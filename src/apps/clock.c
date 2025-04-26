#include "clock.h"

#include "http_manager.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "driver/gptimer.h"

#include "freertos/FreeRTOS.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define TAG "CLOCK"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)

#define TIMER_DIVIDER 80 // 80 MHz / 80 = 1 MHz

typedef struct
{
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint8_t month;
    uint8_t day;
    uint32_t year;
} clock_datetime_t;


static clock_datetime_t current_time = {0};
static gptimer_handle_t timer = NULL; // Timer handle

static void add_seconds(clock_datetime_t* time, uint32_t seconds);

static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    add_seconds(&current_time, 1); // Increment the time by 1 second
    return true;
}


bool fetch_time(clock_datetime_t* time)
{
    LOGD("Fetching time from server...");
    char response[MAX_RESPONSE_LENGTH] = {0};
    size_t response_len = 0;
    if (http_get("http://worldtimeapi.org/api/timezone/America/Los_Angeles", response, &response_len))
    {
        LOGD("Response: %s", response);
        clock_datetime_t datetime = {0};
        char *datetime_field = strstr(response, "\"datetime\":\"");
        if (datetime_field) {
            int matched = sscanf(datetime_field,
                "\"datetime\":\"%4lu-%2hhu-%2hhuT%2lu:%2lu:%2lu",
                &datetime.year, &datetime.month, &datetime.day,
                &datetime.hour, &datetime.minute, &datetime.second);

            if (matched == 6) {
                LOGI("Current time: %04lu-%02hhu-%02hhu %02lu:%02lu:%02lu",
                    datetime.year, datetime.month, datetime.day,
                    datetime.hour, datetime.minute, datetime.second);
                memcpy(time, &datetime, sizeof(clock_datetime_t)); // Copy the parsed time to the provided structure
                return  true; // Parsing succeeded
            } else {
                LOGE("Failed to parse datetime (matched %d of 6 fields)", matched);
                LOGD("Response was: %s", response);
                return false; // Parsing failed
            }
        } else {
            LOGE("Could not find datetime field in response");
            return false; // Field not found
        }
    }
    else
    {
        LOGE("Failed to fetch time from server");
        return false; // HTTP request failed
    }
}

clock_datetime_t get_current_time(void)
{
    return current_time; // Return the current time
}

clock_datetime_t get_current_time12(void)
{
    clock_datetime_t time = current_time;
    if (time.hour > 12) {
        time.hour -= 12; // Convert to 12-hour format
    }
    return time;
}

static void add_seconds(clock_datetime_t* time, uint32_t seconds)
{
    time->second += seconds;
    if (time->second >= 60) {
        time->minute += time->second / 60;
        time->second %= 60;
    }
    if (time->minute >= 60) {
        time->hour += time->minute / 60;
        time->minute %= 60;
    }
    if (time->hour >= 24) {
        time->day += time->hour / 24;
        time->hour %= 24;
    }
}

void clock_init(void)
{
    http_manager_init(); // Initialize HTTP manager

    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer)); // Create a new timer

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000, // 1 second
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config)); // Set the alarm action

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb, // Set the ISR handler
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL)); // Register the event callbacks

    ESP_ERROR_CHECK(gptimer_enable(timer)); // Enable the timer
    ESP_ERROR_CHECK(gptimer_start(timer)); // Start the timer

}

void clock_task(void* pvParameter)
{
    clock_init(); // Initialize the clock
    for (int i = 0; i < 3; i++)
    {
        if (fetch_time(&current_time)) // Fetch the time from the server
        {
            LOGI("Time fetched successfully");
            break;
        }
        else
        {
            LOGE("Failed to fetch time, retrying in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 5 seconds before retrying
        }
    }
    if (current_time.year == 0) // Check if the time was not fetched successfully
    {
        LOGE("Failed to fetch time from server after 3 attempts, using default time");
        current_time.hour = 12;
        current_time.minute = 30;
        current_time.second = 15;
        current_time.month = 1;
        current_time.day = 1;
        current_time.year = 2023; // Default time
    }
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
        LOGI("Current time: %02lu:%02lu:%02lu", current_time.hour, current_time.minute, current_time.second);
    }
}