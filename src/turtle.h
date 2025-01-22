#ifndef _TURTLE_H_
#define _TURTLE_H_

#include <stdint.h>
#include <ti/real.h>

#ifdef __cplusplus
extern "C" {
#endif

#define toInt(var_real_t) os_RealToInt24(&var_real_t)

typedef struct Turtle {
    bool initialized;

    real_t x;
    real_t y;
    real_t angle;
    real_t color;
    real_t pen;
    real_t wrap;

    real_t oldX;
    real_t oldY;
} Turtle;

void Turtle_Initialize(Turtle* t);
void Turtle_Forward(Turtle* t, const real_t* amount);
void Turtle_Right(Turtle* t, const real_t* angle);
void Turtle_Left(Turtle* t, const real_t* angle);
void Turtle_SetAngle(Turtle* t, const real_t* angle);
void Turtle_Goto(Turtle* t, const real_t* x, const real_t* y);
void Turtle_Teleport(Turtle* t, const real_t* x, const real_t* y);
void Turtle_SetPen(Turtle* t, const real_t* pen);
void Turtle_SetColor(Turtle* t, const real_t* color);
void Turtle_SetWrap(Turtle* t, const real_t* wrap);
void Turtle_Draw(Turtle* t);

#ifdef __cplusplus
}
#endif

#endif