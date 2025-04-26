#include "fonts.h"

#include <string.h>

#include "5x3.h"
#include "display_manager.h"

#include "esp_log.h"
#define TAG "FONT"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)


const font_t font_5x3 = {
    .bitmap = font_5x3_chars,
    .width = 3,
    .height = 5,
};


void font_getChar(char c, font_size_E size, uint8_t *bitmap, uint8_t *width, uint8_t *height)
{
    font_t chosenFont;
    switch (size)
    {
        case FONT_SIZE_5x3:
            chosenFont = font_5x3;
            break;
        default:
            chosenFont = font_5x3; // Default to 5x3 if size is not recognized
            break;
    }

    uint8_t index = 0;
    if (c > '0' && c < '9')
    {
        index = (c - '0') * chosenFont.height; // Convert character to index (0-9)
    }

    // Eventually all fonts will be 62 characters long (0-9, A-Z, a-z)
    // So if there is an invalid character make 63 a filled in character for error
    memcpy(bitmap, &chosenFont.bitmap[index], chosenFont.height);
    *width = chosenFont.width;
    *height = chosenFont.height;
    LOGI("Font: %d, Width: %d, Height: %d", size, *width, *height);
}

void font_drawChar(uint8_t x, uint8_t y, char c, font_size_E size, uint32_t color)
{
    uint8_t bitmap[FONT_5X3_WIDTH * FONT_5X3_HEIGHT] = {0};
    uint8_t width = 0;
    uint8_t height = 0;
    font_getChar(c, size, bitmap, &width, &height);
    for (uint8_t row = 0; row < height; row++)
    {
        for (uint8_t col = 0; col < width; col++)
        {
            if (bitmap[row] & (1 << col))
            {
                display_manager_setPixel(y + row, x + (width - col - 1),  color);
            }
        }
    }
}