#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <graphx.h>
#include <string.h>

#include "turtle.h"
#include "static.h"

#define angle_to_rads(a) (a * 0.017453292519943295f)

__attribute__((hot))
float Turtle_fwrapZeroMin(float x, float max) {
    if (x >= 0 && x < max)
        return x;
    return (x >= 0 ? 0 : max) + fmod(x, max);
}

__attribute__((hot))
void Turtle_move(Turtle* t, const float* newXptr, const float* newYptr, bool autoDraw) {
    float newX = *newXptr;
    float newY = *newYptr;

    float oldX = t->X;
    float oldY = t->Y;

    t->X = newX;
    t->Y = newY;

    if (t->Pen > 0 && autoDraw) {
        gfx_SetColor(t->Color);
        gfx_Line(oldX, oldY, newX, newY);
    }

    if (t->Wrap > 0) {
        t->X = Turtle_fwrapZeroMin(newX, GFX_LCD_WIDTH);
        t->Y = Turtle_fwrapZeroMin(newY, GFX_LCD_HEIGHT);

        if (t->Pen > 0 && autoDraw) {
            if (t->X < newX)
                oldX = Turtle_fwrapZeroMin(oldX, GFX_LCD_WIDTH) - GFX_LCD_WIDTH;
            else if (t->X > newX)
                oldX = Turtle_fwrapZeroMin(oldX, GFX_LCD_WIDTH) + GFX_LCD_WIDTH;
            
            if (t->Y < newY)
                oldY = Turtle_fwrapZeroMin(oldY, GFX_LCD_HEIGHT) - GFX_LCD_HEIGHT;
            else if (t->Y > newY)
                oldY = Turtle_fwrapZeroMin(oldY, GFX_LCD_HEIGHT) + GFX_LCD_HEIGHT;

            gfx_Line(oldX, oldY, t->X, t->Y);
        }
    }
}

__attribute__((hot))
void Turtle_Initialize(Turtle *t) {
    t->Initialized = true;

    t->X = GFX_LCD_WIDTH / 2;
    t->Y = GFX_LCD_HEIGHT / 2;
    t->Angle = 0;
    t->Color = 255;
    t->Pen = 1;
    t->Wrap = 1;
    t->SpriteNumber = 0;
    t->SpriteScaleX = 0;
    t->SpriteScaleY = 0;
}

__attribute__((hot))
void Turtle_Forward(Turtle *t, const float* amount, bool autoDraw) {
    float rads = angle_to_rads(t->Angle);
    float newX = t->X + *amount * sinf(rads);
    float newY = t->Y - *amount * cosf(rads);
    
    Turtle_move(t, &newX, &newY, autoDraw);
}

__attribute__((hot))
static inline void clip_angle(Turtle* t) {
    t->Angle = Turtle_fwrapZeroMin(t->Angle, 360);
}

__attribute__((hot))
void Turtle_Right(Turtle* t, const float* angle) {
    t->Angle += *angle;
    clip_angle(t);
}

__attribute__((hot))
void Turtle_Left(Turtle* t, const  float* angle) {
    t->Angle -= *angle;
    clip_angle(t);
}

__attribute__((hot))
void Turtle_SetAngle(Turtle* t, const  float* angle) {
    t->Angle = *angle;
    clip_angle(t);
}

__attribute__((hot))
void Turtle_Goto(Turtle* t, const float* x, const float* y, bool autoDraw) {
    Turtle_move(t, x, y, autoDraw);
}

__attribute__((hot))
void Turtle_Teleport(Turtle* t, const float* x, const float* y, bool autoDraw) {
    float pen = t->Pen;
    t->Pen = 0;
    Turtle_move(t, x, y, autoDraw);
    t->Pen = pen;
}

__attribute__((hot))
void Turtle_SetPen(Turtle* t, const float* pen) {
    t->Pen = *pen;
}

__attribute__((hot))
void Turtle_SetSpriteNumber(Turtle *t, SpriteIndex spriteNumber) {
    t->SpriteNumber = spriteNumber % NumSprites;
}

__attribute__((hot))
void Turtle_SetColor(Turtle* t, const float* color) {
    t->Color = Turtle_fwrapZeroMin(*color, 256.0f);
}

__attribute__((hot))
void Turtle_SetWrap(Turtle *t, const float* wrap) {
    t->Wrap = *wrap;
}

__attribute__((hot))
void Turtle_Draw(Turtle* t, gfx_sprite_t** spriteDictionary) {
    if (!t->Initialized || !t->Pen)
        return;
        
    uint24_t x = (uint24_t)t->X;
    uint24_t y = (uint24_t)t->Y;
    
    if (t->Pen < 0) {
        if (t->SpriteScaleX == 0 || t->SpriteScaleY == 0) {
            gfx_Sprite(spriteDictionary[t->SpriteNumber], x, y);
        } else {
            gfx_ScaledSprite_NoClip(spriteDictionary[t->SpriteNumber], x, y, t->SpriteScaleX, t->SpriteScaleY);
        }
    } else {
        if (x >= 0 && x < GFX_LCD_WIDTH
            && y >= 0 && y < GFX_LCD_HEIGHT) {
            gfx_SetColor(t->Color);
            gfx_SetPixel(x, y);
        }
    }
}