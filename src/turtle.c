#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <graphx.h>
#include <string.h>
#include <debug.h>

#include "turtle.h"
#include "static.h"

#define angle_to_rads(a) (a * 0.017453292519943295f)

float fwrap(float x, float min, float max) {
    if (min > max)
        return fwrap(x, max, min);
    return (x >= 0 ? min : max) + fmod(x, max - min);
}

void move(Turtle* t, const float* newXptr, const float* newYptr)
{
    float newX = *newXptr;
    float newY = *newYptr;

    float oldX = t->x;
    float oldY = t->y;

    t->x = newX;
    t->y = newY;

    if (t->pen)
    {
        gfx_SetColor(t->color);
        gfx_Line(oldX, oldY, newX, newY);
    }

    if (t->wrap)
    {
        t->x = fwrap(newX, 0, GFX_LCD_WIDTH);
        t->y = fwrap(newY, 0, GFX_LCD_HEIGHT);

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

            gfx_Line(oldX, oldY, t->x, t->y);
        }
    }
}

void Turtle_Initialize(Turtle *t)
{
    t->initialized = true;

    t->x = GFX_LCD_WIDTH / 2;
    t->y = GFX_LCD_HEIGHT / 2;
    t->angle = 0;
    t->color = 255;
    t->pen = 0;
    t->wrap = 1;
}

void Turtle_Forward(Turtle *t, const float* amount)
{
    float rads = angle_to_rads(t->angle);
    float newX = t->x + *amount * sinf(rads);
    float newY = t->y - *amount * cosf(rads);
    
    move(t, &newX, &newY);
}

static inline void clip_angle(Turtle* t)
{
    t->angle = fwrap(t->angle, 0, 360);
}

void Turtle_Right(Turtle* t, const float* angle)
{
    t->angle -= *angle;
    clip_angle(t);
}

void Turtle_Left(Turtle* t, const  float* angle)
{
    t->angle += *angle;
    clip_angle(t);
}

void Turtle_SetAngle(Turtle* t, const  float* angle)
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
    if (!t->initialized || !t->pen)
        return;

    uint24_t x = (uint24_t)t->x;
    uint24_t y = (uint24_t)t->y;
    uint8_t color = (uint8_t)fwrap(t->color, 0, 256.0f);

    if (x >= 0 && x < GFX_LCD_WIDTH
        && y >= 0 && y < GFX_LCD_HEIGHT)
    {
        gfx_SetColor(color);
        gfx_SetPixel(x, y);
    }
}