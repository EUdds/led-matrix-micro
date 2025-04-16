#pragma once

#include "fonts/5x3.h"

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


const font_t font_5x3 = {
    .bitmap = font_5x3_chars,
    .width = 3,
    .height = 5,
};



void font_getChar(char c, font_size_E size, uint8_t *bitmap, uint8_t *width, uint8_t *height);