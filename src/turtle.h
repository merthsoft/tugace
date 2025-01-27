#ifndef _TURTLE_H_
#define _TURTLE_H_

#include <stdint.h>
#include <ti/real.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NumTurtles      10
#define MaxLineBuffer   NumTurtles*2*5

extern size_t Turtle_LineBufferIndex;
extern float Turtle_LineBuffer[MaxLineBuffer];

typedef struct Turtle {
    bool initialized;
    
    float x;
    float y;
    float angle;
    float color;
    float pen;
    float wrap;
} Turtle;

void Turtle_StartEngine();
void Turtle_Initialize(Turtle* t);
void Turtle_Forward(Turtle* t, const float* amount);
void Turtle_Right(Turtle* t, const float* angle);
void Turtle_Left(Turtle* t, const float* angle);
void Turtle_SetAngle(Turtle* t, const float* angle);
void Turtle_Goto(Turtle* t, const float* x, const float* y);
void Turtle_Teleport(Turtle* t, const float* x, const float* y);
void Turtle_SetPen(Turtle* t, const float* pen);
void Turtle_SetColor(Turtle* t, const float* color);
void Turtle_SetWrap(Turtle* t, const float* wrap);
void Turtle_Draw(Turtle* t);

#ifdef __cplusplus
}
#endif

#endif