#include "graphics.h"

#include "display_manager.h"


void font_getChar(char c, font_size_E size, uint8_t *bitmap, uint8_t *width, uint8_t *height)
{
    switch (size)
    {
        case FONT_SIZE_5x3:
            if (c >= '0' && c <= '9')
            {
                uint8_t index = c - '0'; // Convert character to index (0-9)
                *bitmap = font_5x3.bitmap[index]; // Get the bitmap for the character
                *width = font_5x3.width; // Set the width
                *height = font_5x3.height; // Set the height
            }
            break;
        default:
            *width = 0;
            *height = 0;
            break;
    }
}