#include <stdbool.h>
#include <stdint.h>
#include <gui/canvas.h>

/*  7px 3 width digit font by Sefjor
 * digit encoding example
 *7 ███ 111
 *6 █ █ 101
 *5 █ █ 101
 *4 █ █ 101
 *3 █ █ 101
 *2 █ █ 101
 *1 ███ 111
 *0     000 this string is empty, used to align
 *     ↓ ↓ ↓
 *   FE 82 FE  //0
 */

static uint8_t font[10][3] = {
    {0xFE, 0x82, 0xFE}, // 0;
    {0x00, 0xFE, 0x00}, // 1;
    {0xF2, 0x92, 0x9E}, // 2;
    {0x92, 0x92, 0xFE}, // 3;
    {0x1E, 0x10, 0xFE}, // 4;
    {0x9E, 0x92, 0xF2}, // 5;
    {0xFE, 0x92, 0xF2}, // 6;
    {0x02, 0x02, 0xFE}, // 7;
    {0xFE, 0x92, 0xFE}, // 8;
    {0x9E, 0x92, 0xFE}, // 9;
};

#define FONT_HEIGHT 7
#define FONT_WIDTH 3

static void DrawPoint2048(Canvas* const canvas, uint8_t x, uint8_t y, bool is_filled) {
    //point draw here
    if (is_filled) {
        canvas_set_color(canvas, ColorBlack);
    } else {
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_dot(canvas, x, y);
}

static void _DrawColumn2048(Canvas* const canvas, uint8_t digit, uint8_t x, uint8_t y, uint8_t column) {
    for(uint8_t offset = 0; offset < FONT_HEIGHT; offset++) {
        bool is_filled = (font[digit][column] >> offset) & 0x1;
        DrawPoint2048(canvas, x, y + offset, is_filled);
    }
    DrawPoint2048(canvas, x, y + 8, false);
}

static uint8_t _DrawDigit2048(Canvas* const canvas, uint8_t digit, uint8_t coord_x, uint8_t coord_y) {
    uint8_t x_shift = 0;

    if(digit != 1) {
        for(int column = 0; column < FONT_WIDTH; column++) {
            _DrawColumn2048(canvas, digit, coord_x + column, coord_y, column);
        }
        x_shift = 3;
    } else {
        _DrawColumn2048(canvas, digit, coord_x, coord_y, true);
        x_shift = 1;
    }

    return x_shift;
}

static void _DrawEmptyColumn2048(Canvas* const canvas, uint8_t coord_x, uint8_t coord_y) {
    for(int y = 0; y < 9; ++y) {
        DrawPoint2048(canvas, coord_x, coord_y + y, false);
    }
}

/* We drawing text field with 1px white border
 * at given coords. Total size is:
 *  x = 9 = 1 + 7 + 1
 *  y = 1 + total text width + 1
 */

/*
 * Returns array of digits and it's size,
 * digits should be at least 4 size
 * works from 1 to 9999
 */
static void _ParseNumber2048(uint16_t number, uint8_t* digits, uint8_t* size) {
    *size = 0;
    uint16_t divider = 1000;
    //find first digit, result is highest divider
    while(number / divider == 0) {
        divider /= 10;
        if(divider == 0) {
            break;
        }
    }

    for (int i = 0; divider != 0; i++) {
        digits[i] = number / divider;
        number %= divider;
        *size += 1;
        divider /= 10;       
    }
}

void DrawNumberFor2048(Canvas* const canvas, uint8_t coord_x, uint8_t  coord_y, uint16_t number) {
    _DrawEmptyColumn2048(canvas, coord_x, coord_y); //x = 0
    coord_x++;

    uint8_t digits[4];
    uint8_t size;
    _ParseNumber2048(number, digits, &size);
    for(uint8_t i = 0; i < size; i++) {
        coord_x += _DrawDigit2048(canvas, digits[i], coord_x, coord_y);
        _DrawEmptyColumn2048(canvas, coord_x, coord_y);
        coord_x++;
    }
}
