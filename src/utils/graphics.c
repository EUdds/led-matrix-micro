#include "graphics.h"

#include "display_manager.h"
#include "fonts.h"

#include "esp_log.h"
#include <string.h>
#include <stdbool.h>

#define TAG "GRAPHICS"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)


void graphics_drawChar(uint8_t x, uint8_t y, char c, font_size_E size, uint32_t color)
{
    font_drawChar(x, y, c, size, color);
    LOGD("Drawing character '%c' at (%d, %d) with size %d and color %06lX", c, x, y, size, color);
}

void graphics_drawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t color)
{
    for (uint8_t row = 0; row < height; row++)
    {
        for (uint8_t col = 0; col < width; col++)
        {
            display_manager_setPixel(y + row, x + col, color);
        }
    }
    LOGI("Drawing rectangle at (%d, %d) with width %d, height %d and color %06lX", x, y, width, height, color);
}

void graphics_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint32_t color)
{
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        display_manager_setPixel(y1, x1, color); // Set pixel at (x1, y1)

        if (x1 == x2 && y1 == y2) break; // Line is complete

        int err2 = err * 2;
        if (err2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}