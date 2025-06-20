#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifndef NEOPIXEL_NUM_ROWS
#define NEOPIXEL_NUM_ROWS (8)
#endif

#ifndef NEOPIXEL_NUM_COLS
#define NEOPIXEL_NUM_COLS (32)
#endif

#define DISPLAY_WIDTH NEOPIXEL_NUM_COLS
#define DISPLAY_HEIGHT NEOPIXEL_NUM_ROWS


#define MAX_DISPLAY_BUFFERS 8

typedef enum
{
    DISPLAY_ROTATION_0 = 0,
    DISPLAY_ROTATION_90 = 1,
    DISPLAY_ROTATION_180 = 2,
    DISPLAY_ROTATION_270 = 3,
} displayManager_rotation_E;

typedef enum
{
    DISPLAY_MANAGER_LAYER_BACKGROUND = 0,
    DISPLAY_MANAGER_LAYER_FOREGROUND = 1,
    DISPLAY_MANGER_LAYER_POPUP = 2,
    DISPLAY_MANAGER_LAYER_SYSTEM = 3,
} displayManager_layer_E;

typedef struct
{
    uint32_t* buffer;
    uint32_t width;
    uint32_t height;
    uint32_t x;          // X position on display
    uint32_t y;          // Y position on display
    displayManager_layer_E layer;
    bool active;
    uint8_t opacity;     // 0-255 for transparency
    const char* owner;   // Name of the app that owns this buffer
} displayManager_buffer_t;



// Buffer management functions
esp_err_t display_manager_init(void);
displayManager_buffer_t* display_manager_create_buffer(const char* owner_name, 
                                                     uint32_t width, 
                                                     uint32_t height,
                                                     uint32_t x,
                                                     uint32_t y,
                                                     displayManager_layer_E layer);
void display_manager_free_buffer(displayManager_buffer_t* buffer);
void display_manager_setBufferPixel(displayManager_buffer_t* buffer, 
                                          uint32_t x, 
                                          uint32_t y, 
                                          uint32_t color);

// Existing functions
void display_manager_setRawPixel(uint32_t row, uint32_t col, uint32_t color);
void display_manager_setBrightness(float brightness);
void display_manager_clearScreen(void);
void display_manager_task(void* pvParameter);