#pragma once

#include <stdint.h>

#ifndef NEOPIXEL_NUM_ROWS
#define NEOPIXEL_NUM_ROWS (8)
#endif

#ifndef NEOPIXEL_NUM_COLS
#define NEOPIXEL_NUM_COLS (32)
#endif

typedef enum
{
    DISPLAY_ROTATION_0 = 0,
    DISPLAY_ROTATION_90 = 1,
    DISPLAY_ROTATION_180 = 2,
    DISPLAY_ROTATION_270 = 3,
} displayManager_rotation_E;


void display_manager_setPixel(uint32_t row, uint32_t col, uint32_t color);
void display_manager_setBrightness(float brightness);

void display_manager_task(void* pvParameter);