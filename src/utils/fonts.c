#include "fonts.h"
#include "telnet_log.h"
#include "display_manager.h"

#include <string.h>

#include "5x3.h"

#include "esp_log.h"
#define TAG "FONT"


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
    LOGD("Font: %d, Width: %d, Height: %d", size, *width, *height);
}

void font_drawChar(displayManager_buffer_t* buffer, uint8_t x, uint8_t y,
                  char c, font_size_E size, uint32_t color)
{
    if (!buffer || !buffer->active) {
        LOGE("Invalid or inactive buffer");
        return;
    }

    uint8_t bitmap[8];  // Temporary storage for character bitmap
    uint8_t width, height;

    // Get the character bitmap and dimensions
    font_getChar(c, size, bitmap, &width, &height);

    // Check if character fits in the buffer
    if (x + width > buffer->width || y + height > buffer->height) {
        LOGE("Character '%c' does not fit in buffer at (%d, %d) with size (%d, %d)", 
             c, x, y, width, height);
        return;
    }

    // Draw the character pixel by pixel
    for (uint8_t row = 0; row < height; row++) {
        uint8_t line = bitmap[row];
        for (uint8_t col = 0; col < width; col++)
        {
            if (line & (1 << (width - 1 - col))) { // Check if pixel is set
                display_manager_setBufferPixel(buffer, x + col, y + row, color);
            } else {
                display_manager_setBufferPixel(buffer, x + col, y + row, 0); // Clear pixel
            }
        }
    }
}