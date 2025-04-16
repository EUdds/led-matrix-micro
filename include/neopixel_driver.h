#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "neopixel.h"

void neopixel_task(void* pvParameter);
void neopixel_driver_addToQueue(tNeopixel* pixel, uint32_t pixelCount);
void neopixel_driver_setRawPixel(tNeopixel* pixel);
void neopixel_driver_fill_matrix(uint32_t rgb);
void neopixel_driver_setBrightness(float b);
void neopixel_driver_setPixel(int index, uint32_t color);