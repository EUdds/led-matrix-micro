#pragma once

#include <stdint.h>
#include "display_manager.h"
#include "fonts.h"

// Drawing functions that work with display buffers
void graphics_drawChar(displayManager_buffer_t* buffer, 
                      uint8_t x, uint8_t y, 
                      char c, 
                      font_size_E size, 
                      uint32_t color);

void graphics_drawRectangle(displayManager_buffer_t* buffer,
                          uint8_t x, uint8_t y,
                          uint8_t width, uint8_t height,
                          uint32_t color);

void graphics_drawLine(displayManager_buffer_t* buffer,
                      uint8_t x1, uint8_t y1,
                      uint8_t x2, uint8_t y2,
                      uint32_t color);

// Add more graphics functions as needed...