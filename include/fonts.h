#pragma once
#include "5x3.h"

#include <stdint.h>

typedef enum
{
    FONT_SIZE_5x3 = 0
} font_size_E;


typedef struct
{
    const uint8_t *bitmap; // Pointer to the bitmap data
    uint8_t width;   // Width of the character in pixels
    uint8_t height;  // Height of the character in pixels
} font_t;


void font_getChar(char c, font_size_E size, uint8_t *bitmap, uint8_t *width, uint8_t *height);
void font_drawChar(uint8_t x, uint8_t y, char c, font_size_E size, uint32_t color);