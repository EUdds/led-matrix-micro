#include "graphics.h"
#include "esp_log.h"
#include <string.h>
#include <stdbool.h>
#include "telnet_log.h"

#define TAG "GRAPHICS"

// Helper function to check if coordinates are within buffer bounds
static bool is_in_bounds(displayManager_buffer_t* buffer, uint8_t x, uint8_t y) {
    return (buffer != NULL && 
            x < buffer->width && 
            y < buffer->height);
}

void graphics_drawChar(displayManager_buffer_t* buffer,
                      uint8_t x, uint8_t y,
                      char c,
                      font_size_E size,
                      uint32_t color)
{
    if (!buffer || !buffer->active) {
        LOGE("Invalid or inactive buffer");
        return;
    }
    LOGD("Drawing character '%c' at (%d, %d) with size %d and color %06lX", 
         c, x, y, size, color);

    font_drawChar(buffer, x, y, c, size, color);
}

void graphics_drawRectangle(displayManager_buffer_t* buffer,
                          uint8_t x, uint8_t y,
                          uint8_t width, uint8_t height,
                          uint32_t color)
{
    if (!buffer || !buffer->active) {
        LOGE("Invalid or inactive buffer");
        return;
    }

    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t col = 0; col < width; col++) {
            if (is_in_bounds(buffer, x + col, y + row)) {
                display_manager_setBufferPixel(buffer, x + col, y + row, color);
            }
        }
    }

    LOGI("Drawing rectangle in buffer '%s' at (%d, %d) with width %d, height %d", 
         buffer->owner, x, y, width, height);
}

void graphics_drawLine(displayManager_buffer_t* buffer,
                      uint8_t x1, uint8_t y1,
                      uint8_t x2, uint8_t y2,
                      uint32_t color)
{
    if (!buffer || !buffer->active) {
        LOGE("Invalid or inactive buffer");
        return;
    }

    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (is_in_bounds(buffer, x1, y1)) {
            display_manager_setBufferPixel(buffer, x1, y1, color);
        }

        if (x1 == x2 && y1 == y2) break;

        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }

    LOGI("Drawing line in buffer '%s' from (%d, %d) to (%d, %d)", 
         buffer->owner, x1, y1, x2, y2);
}