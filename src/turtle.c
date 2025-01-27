#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <graphx.h>
#include <string.h>
#include <debug.h>

#include "turtle.h"
#include "static.h"

#define angle_to_rads(a) (a * 0.017453292519943295f)

size_t Turtle_LineBufferIndex;
float Turtle_LineBuffer[MaxLineBuffer];

static inline void pushLineBuffer(float color, float oldX, float oldY, float newX, float newY) {
    Turtle_LineBuffer[Turtle_LineBufferIndex++] = color;
    Turtle_LineBuffer[Turtle_LineBufferIndex++] = oldX;
    Turtle_LineBuffer[Turtle_LineBufferIndex++] = oldY;
    Turtle_LineBuffer[Turtle_LineBufferIndex++] = newX;
    Turtle_LineBuffer[Turtle_LineBufferIndex++] = newY;
}

float fwrap(float x, float min, float max) {
    return min > max
            ? (x >= 0 ? max : min) + fmodf(x, min - max)
            : (x >= 0 ? min : max) + fmodf(x, max - min);
}

void move(Turtle* t, const float* x, const float* y)
{
    float newX = *x;
    float newY = *y; 
    float oldX = t->x;
    float oldY = t->y;
    if (t->pen)
        pushLineBuffer(t->color, oldX, oldY, newX, newY);

    t->x = newX;
    t->y = newY;

    if (t->wrap)
    {
        t->x = fwrap(t->x, 0, GFX_LCD_WIDTH);
        t->y = fwrap(t->x, 0, GFX_LCD_HEIGHT);

        if (t->pen)
        {
            if (t->x < newX)
                oldX = fwrap(oldX, 0, GFX_LCD_WIDTH) - GFX_LCD_WIDTH;
            else if (t->x > newX)
                oldX = fwrap(oldX, 0, GFX_LCD_WIDTH) + GFX_LCD_WIDTH;
            
            if (t->y < newY)
                oldY = fwrap(oldY, 0, GFX_LCD_HEIGHT) - GFX_LCD_HEIGHT;
            else if (t->y > newY)
                oldY = fwrap(oldY, 0, GFX_LCD_HEIGHT) + GFX_LCD_HEIGHT;

            pushLineBuffer(t->color, oldX, oldY, newX, newY);
        }
    }
}

void Turtle_StartEngine()
{
    Turtle_LineBufferIndex = 0;
    memset(Turtle_LineBuffer, 0, MaxLineBuffer * sizeof(float));
}

void Turtle_Initialize(Turtle *t)
{
    t->initialized = true;

    t->x = GFX_LCD_WIDTH / 2;
    t->y = GFX_LCD_HEIGHT / 2;

    t->color = 255;
    t->pen = 1;
    t->wrap = 1;
}

void Turtle_Forward(Turtle *t, const float* amount)
{
    int angle = (int)t->angle;
    float newX = t->x + *amount * sinf(angle);
    float newY = t->y + *amount * -sinf(angle);

    move(t, &newX, &newY);
}

void clip_angle(Turtle* t)
{
    t->angle = fwrap(t->angle, 0, 360);
}

void Turtle_Right(Turtle* t, float* angle)
{
    t->angle -= *angle;
    clip_angle(t);
}

void Turtle_Left(Turtle* t, float* angle)
{
    t->angle += *angle;
    clip_angle(t);
}

void Turtle_SetAngle(Turtle* t, float* angle)
{
    t->angle = *angle;
    clip_angle(t);
}

void Turtle_Goto(Turtle* t, const float* x, const float* y)
{
    move(t, x, y);
}

void Turtle_Teleport(Turtle* t, const float* x, const float* y)
{
    float pen = t->pen;
    t->pen = 0;
    move(t, x, y);
    t->pen = pen;
}

void Turtle_SetPen(Turtle* t, const float* pen)
{
    t->pen = *pen;
}

void Turtle_SetColor(Turtle* t, const float* color)
{
    t->color = *color;
}

void Turtle_SetWrap(Turtle *t, const float* wrap)
{
    t->wrap = *wrap;
}

void Turtle_Draw(Turtle* t)
{
    if (!t->initialized)
        return;

    for (size_t i = 0; i < Turtle_LineBufferIndex; i++)
    {
        gfx_SetColor(Turtle_LineBuffer[i++]);
        gfx_Line(
            Turtle_LineBuffer[i++], 
            Turtle_LineBuffer[i++], 
            Turtle_LineBuffer[i++], 
            Turtle_LineBuffer[i++]);
    }
    Turtle_LineBufferIndex = 0;

    uint24_t x = (uint24_t)t->x;
    uint24_t y = (uint24_t)t->y;
    uint8_t color = (uint8_t)fmodf(t->color, 256.0f);

    if (x >= 0 && x < GFX_LCD_WIDTH
        && y >= 0 && y < GFX_LCD_HEIGHT)
    {
        gfx_SetColor(color);
        gfx_SetPixel(x, y);
    }
}