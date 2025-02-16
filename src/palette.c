#include <sys/util.h>

#include "palette.h"

void Palette_FadeOut(uint16_t* base_palette, uint8_t start, uint8_t length, uint8_t step) {
    if (step == 0)
        step = 1;
    for (int amount = 255; amount >= 0; amount -= step)     {
        for (int i = start; i <= start + length; i++)
            DevicePaletteLocation[i] = gfx_Darken(base_palette[i], amount);
    }
}

void Pallete_FadeIn(uint16_t* base_palette, uint8_t start, uint8_t length, uint8_t step) {
    if (step == 0)
        step = 1;
    for (int amount = 0; amount < 256; amount += step) {
        for (int i = start; i <= start + length; i++)
            DevicePaletteLocation[i] = gfx_Darken(base_palette[i], amount);
    }
}

uint16_t Palette_HsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (s == 0) {
        r = v;
        g = v;
        b = v;
        return gfx_RGBTo1555(r, g, b);
    }

    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6; 

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            r = v; g = t; b = p;
            break;
        case 1:
            r = q; g = v; b = p;
            break;
        case 2:
            r = p; g = v; b = t;
            break;
        case 3:
            r = p; g = q; b = v;
            break;
        case 4:
            r = t; g = p; b = v;
            break;
        default:
            r = v; g = p; b = q;
            break;
    }

    return gfx_RGBTo1555(r, g, b);
}

void Palette_Shift(uint16_t* palette) {
    palette[255] = palette[1];
    for (int i = 1; i < 255; i++)
        palette[i] = palette[i+1];
}

void Palette_Default(uint16_t* palette) {
    for (uint16_t i = 0; i <= 255; i++)
    {       
        uint16_t c = (i << 8) | i;
        
        uint8_t  r = (c >> 11) & 0b11111;
        uint8_t  g = (c >>  6) & 0b11111;
        uint8_t  b = (c >>  0) & 0b11111;

        palette[i] = (r << 10) | (g << 5) | b;
    }
}

void Palette_Rainbow(uint16_t* palette) {
    for (uint16_t i = 1; i < 256; i++)
        palette[i] = Palette_HsvToRgb(i - 1, 255, 255);
}

void Palette_Value(uint16_t* palette, uint8_t color, uint8_t direction) {
    int index = direction ? 255 : 1;
    int delta = direction ? -1 : 1;
    for (uint8_t i = 1; i <= 255; i++)
    {
        palette[index] = Palette_HsvToRgb(color, 255, i - 1);
        index += delta;
    }
}

void Palette_Saturation(uint16_t* palette, uint8_t color, uint8_t direction) {
    int index = direction ? 255 : 1;
    int delta = direction ? -1 : 1;
    for (uint8_t i = 1; i <= 255; i++)
    {
        palette[index] = Palette_HsvToRgb(color, i - 1, 255);
        index += delta;
    }
}

void Palette_Gray(uint16_t* palette, uint8_t direction) {
    int index = direction ? 255 : 1;
    int delta = direction ? -1 : 1;
    for (uint8_t i = 1; i <= 255; i++)
    {
        palette[index] = gfx_RGBTo1555(i, i, i);
        index += delta;
    }
}

void Palette_Random(uint16_t* palette) {
    for (int i = 1; i < 256; i++)
        palette[i] = Palette_HsvToRgb(random() % 256, 255, 255);
}

void Palette_Spectrum(uint16_t* palette, uint8_t color, int hueSkip) {
    uint8_t i = 1;
    int hue = color;
    int val = 128;
    while (i <= 255) {
        palette[i++] = Palette_HsvToRgb(hue, 255, val);
        val+=4;
        if (val >= 256)
        {
            val = 128;
            hue = (hue + 4 * hueSkip) % 256;
        }
    }
}