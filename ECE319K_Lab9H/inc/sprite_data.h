#pragma once
#include <stdint.h>

#define SPRITE_W 32
#define SPRITE_H 32

#define SPRITE_TRANSPARENT_COLOR 0xF81F
//void ST7735_DrawBitmap(int16_t x, int16_t y, const uint16_t *image, int16_t w, int16_t h);

struct SPRITE_ARRAY {
    const uint16_t *arr;
    const int16_t WIDTH;
    const int16_t HEIGHT;
};

struct SpriteSet {
    const SPRITE_ARRAY* idle;
    const SPRITE_ARRAY* punch;
    const SPRITE_ARRAY* kick;
    const SPRITE_ARRAY* dodge;
};

extern const SpriteSet CHAR1_SPRITES;

extern const SpriteSet CHAR2_SPRITES;