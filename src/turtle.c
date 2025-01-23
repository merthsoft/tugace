#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <graphx.h>
#include <string.h>
#include <debug.h>

#include "turtle.h"
#include "static.h"

#define angle_to_rads(a) (a * 0.017453292519943295f)

void fwrap(real_t* x, const real_t* min, const real_t* max) {
    return;
    if (os_RealCompare(x, min) >= 0 && os_RealCompare(x, max) <= 0)
        return;
    
    real_t diff = os_RealSub(max, min);
    if (os_RealCompare(&diff, &STATIC_REAL_0) < 0)
        diff = os_RealNeg(&diff);
    real_t mod = os_RealMod(x, &diff);
    
    *x = os_RealAdd(x >= 0 ? min : max, &mod);
}

void move(Turtle* t, const real_t* newX, const real_t* newY)
{
    t->oldX = t->x;
    t->oldY = t->y;

    t->x = *newX;
    t->y = *newY;

    // if (toInt(t->wrap))
    // {
    //     fwrap(&t->x, 0, &STATIC_REAL_GFX_LCD_WIDTH);
    //     fwrap(&t->y, 0, &STATIC_REAL_GFX_LCD_HEIGHT);

    //     if (toInt(t->pen))
    //     {
    //         if (os_RealCompare(&t->x, newX) < 0)
    //         {
    //             fwrap(&t->oldX, &STATIC_REAL_0, &STATIC_REAL_GFX_LCD_WIDTH);
    //             t->oldX = os_RealSub(&t->oldX, &STATIC_REAL_GFX_LCD_WIDTH);
    //         }
    //         else if (os_RealCompare(&t->x, newX) > 0)
    //         {
    //             fwrap(&t->oldX, &STATIC_REAL_0, &STATIC_REAL_GFX_LCD_WIDTH);
    //             t->oldX = os_RealAdd(&t->oldX, &STATIC_REAL_GFX_LCD_WIDTH);
    //         }

    //         if (os_RealCompare(&t->y, newY) < 0)
    //         {
    //             fwrap(&t->oldY, &STATIC_REAL_0, &STATIC_REAL_GFX_LCD_HEIGHT);
    //             t->oldY = os_RealSub(&t->oldY, &STATIC_REAL_GFX_LCD_HEIGHT);
    //         }
    //         else if (os_RealCompare(&t->y, newY) > 0)
    //         {
    //             fwrap(&t->oldY, &STATIC_REAL_0, &STATIC_REAL_GFX_LCD_HEIGHT);
    //             t->oldY = os_RealAdd(&t->oldY, &STATIC_REAL_GFX_LCD_HEIGHT);
    //         }

    //     }
    // }
}

void Turtle_Initialize(Turtle *t)
{
    t->initialized = true;

    t->x = t->oldX = STATIC_REAL_GFX_LCD_WIDTH_HALF;
    t->y = t->oldY = STATIC_REAL_GFX_LCD_HEIGHT_HALF;

    t->color = STATIC_REAL_255;
    t->pen = STATIC_REAL_1;
}

void Turtle_Forward(Turtle *t, const real_t* amount)
{
    real_t degs = os_RealDegToRad(&t->angle);
    real_t deltaX = os_RealSinRad(&degs);
    real_t deltaY = os_RealCosRad(&degs);
    
    deltaX = os_RealMul(amount, &deltaX);
    deltaY = os_RealMul(amount, &deltaY);

    deltaX = os_RealAdd(&deltaX, &t->x);
    deltaY = os_RealAdd(&deltaY, &t->y);
    move(t, &deltaX, &deltaY);
}

void clip_angle(Turtle* t)
{
    fwrap(&t->angle, &STATIC_REAL_0, &STATIC_REAL_360);
}

void Turtle_Right(Turtle* t, const real_t* angle)
{
    t->angle = os_RealSub(&t->angle, angle);
    clip_angle(t);
}

void Turtle_Left(Turtle* t, const real_t* angle)
{
    t->angle = os_RealAdd(&t->angle, angle);
    clip_angle(t);
}

void Turtle_SetAngle(Turtle* t, const real_t* angle)
{
    t->angle = *angle;
    clip_angle(t);
}

void Turtle_Goto(Turtle* t, const real_t* x, const real_t* y)
{
    move(t, x, y);
}

void Turtle_Teleport(Turtle* t, const real_t* x, const real_t* y)
{
     move(t, x, y);
     t->oldX = t->x;
     t->oldY = t->y;
}

void Turtle_SetPen(Turtle* t, const real_t* pen)
{
    t->pen = *pen;
}

void Turtle_SetColor(Turtle* t, const real_t* color)
{
    t->color = *color;
}

void Turtle_SetWrap(Turtle *t, const real_t *wrap)
{
    t->wrap = *wrap;
}

void Turtle_Draw(Turtle* t)
{
    if (!t->initialized)
        return;

    uint24_t x = toInt(t->x);
    uint24_t y = toInt(t->y);
    uint24_t color = toInt(t->color);

    if (x >= 0 && x < GFX_LCD_WIDTH
        && y >= 0 && y < GFX_LCD_HEIGHT)
    {
        gfx_SetColor(color);
        gfx_SetPixel(x, y);

        //if (os_RealCompare(&t->pen, &STATIC_REAL_0) > 0)
        {
            uint24_t oldX = toInt(t->oldX);
            uint24_t oldY = toInt(t->oldY);
            
            //if (oldX != x && oldY != y) {
                gfx_Line(oldX, oldY, x, y);
            //}
        }
    }
}