#pragma once

#include "fonts.h"

void graphics_drawChar(uint8_t x, uint8_t y, char c, font_size_E size, uint32_t color);
void graphics_drawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t color);
void graphics_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint32_t color);