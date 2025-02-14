#ifndef _PALETTE_H_
#define _PALETTE_H_

#include <stdint.h>

#include <graphx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DevicePaletteLocation gfx_palette

uint16_t Pallete_HsvToRgb(uint8_t h, uint8_t s, uint8_t v);

void Palette_FadeOut(uint16_t* base_palette, uint8_t start, uint8_t length, uint8_t step);
void Pallete_FadeIn(uint16_t* base_palette, uint8_t start, uint8_t length, uint8_t step);

void Palette_Shift(uint16_t* palette);
void Palette_Default(uint16_t* palette);
void Palette_Rainbow(uint16_t* palette);
void Palette_Value(uint16_t* palette, uint8_t color, uint8_t direction);
void Palette_Saturation(uint16_t* palette, uint8_t color, uint8_t direction);
void Palette_Gray(uint16_t* palette, uint8_t direction);
void Palette_Random(uint16_t* palette);
void Palette_Spectrum(uint16_t* palette, uint8_t color, int hueSkip);

#ifdef __cplusplus
}
#endif

#endif