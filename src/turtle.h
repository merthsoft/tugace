#ifndef _TURTLE_H_
#define _TURTLE_H_
#include <graphx.h>
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
    uint8_t SpriteScaleX;
    uint8_t SpriteScaleY;
} Turtle;

void Turtle_Initialize(Turtle* t);
void Turtle_Forward(Turtle* t, const float* amount, bool autoDraw);
void Turtle_Right(Turtle* t, const float* angle);
void Turtle_Left(Turtle* t, const float* angle);
void Turtle_SetAngle(Turtle* t, const float* angle);
void Turtle_Goto(Turtle* t, const float* x, const float* y, bool autoDraw);
void Turtle_Teleport(Turtle* t, const float* x, const float* y, bool autoDraw);
void Turtle_SetPen(Turtle* t, const float* pen);
void Turtle_SetSpriteNumber(Turtle* t, SpriteIndex spriteNumber);
void Turtle_SetColor(Turtle* t, const float* color);
void Turtle_SetWrap(Turtle* t, const float* wrap);

void Turtle_Draw(Turtle* t, gfx_sprite_t** spriteDictionary);

#ifdef __cplusplus
}
#endif

#endif