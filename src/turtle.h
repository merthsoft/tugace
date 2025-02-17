#ifndef _TURTLE_H_
#define _TURTLE_H_

#include <stdint.h>

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) Turtle {
    bool Initialized;
    
    float X;
    float Y;
    float Angle;
    float Color;
    float Pen;
    float Wrap;
    SpriteIndex SpriteNumber;
} Turtle;

void Turtle_Initialize(Turtle* t);
void Turtle_Forward(Turtle* t, const float* amount, bool autoDraw);
void Turtle_Right(Turtle* t, const float* angle);
void Turtle_Left(Turtle* t, const float* angle);
void Turtle_SetAngle(Turtle* t, const float* angle);
void Turtle_Goto(Turtle* t, const float* x, const float* y, bool autoDraw);
void Turtle_Teleport(Turtle* t, const float* x, const float* y, bool autoDraw);
void Turtle_SetPen(Turtle* t, const float* pen);
void Turtle_SetSpriteNumber(Turtle* t, uint8_t spriteNumber);
void Turtle_SetColor(Turtle* t, const float* color);
void Turtle_SetWrap(Turtle* t, const float* wrap);

__attribute__((hot))
static inline void Turtle_Draw_InLine(Turtle* t, gfx_sprite_t** spriteDictionary) {
    if (!t->Initialized || !t->Pen)
        return;

    uint24_t x = (uint24_t)t->X;
    uint24_t y = (uint24_t)t->Y;
    
    if (t->Pen < 0) {
        gfx_Sprite(spriteDictionary[t->SpriteNumber], x, y);
    } else {
        if (x >= 0 && x < GFX_LCD_WIDTH
            && y >= 0 && y < GFX_LCD_HEIGHT) {
            gfx_SetColor(t->Color);
            gfx_SetPixel(x, y);
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif