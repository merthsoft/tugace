#ifndef _INLINE_H_
#define _INLINE_H_

#include <string.h>

#include "static.h"
#include "turtle.h"

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((hot))
static inline void PushTurtle_Inline(float* stack, StackPointer* stackPointer, const Turtle* turtle) {
    memcpy(&stack[*stackPointer], &turtle->X, sizeof(float)*NumDataFields);
    *stackPointer = *stackPointer + NumDataFields;
}

__attribute__((hot))
static inline void PopTurtle_InLine(float* stack, StackPointer* stackPointer, Turtle* turtle) {
    *stackPointer = *stackPointer - NumDataFields;
    memcpy(&turtle->X, &stack[*stackPointer], sizeof(float)*NumDataFields);
    #ifdef DEBUG
    memset(&stack[*stackPointer], 0, sizeof(float)*NumDataFields);
    #endif
}

__attribute__((hot))
static inline void PeekTurtle_InLine(float* stack, StackPointer* stackPointer, Turtle* turtle) {
    memcpy(&turtle->X, &stack[*stackPointer - NumDataFields], sizeof(float)*NumDataFields);
}

__attribute__((hot))
static inline void Push_InLine(float* stack, StackPointer* stackPointer, const float* value) {
    stack[*stackPointer] = *value;
    *stackPointer = *stackPointer + 1;
}

__attribute__((hot))
static inline float Pop_InLine(float* stack, StackPointer* stackPointer) {
    *stackPointer = *stackPointer - 1;
    float ret = stack[*stackPointer];
    #ifdef DEBUG
    stack[*stackPointer] = 0;
    #endif
    return ret;
}

__attribute__((hot))
static inline float Peek_InLine(float* stack, StackPointer* stackPointer) {
    return stack[*stackPointer - 1];
}

// http://www.cse.yorku.ca/~oz/hash.html
__attribute__((hot))
static inline uint24_t Hash_InLine(const ProgramToken* arr, size_t length) {
    uint32_t hash = 5381;
    uint8_t c;
    const uint8_t* d = (uint8_t*)arr;

    while (length--) {
        c = *d++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

__attribute__((hot))
static inline TugaOpCode GetOpCodeFromHash_Inline(uint24_t hash)
{
    switch (hash) {
        case Hash_COLOR:
            return toc_COLOR;
        case Hash_PEN:
            return toc_PEN;
        case Hash_FORWARD:
            return toc_FORWARD;
        case Hash_LEFT:
            return toc_LEFT;
        case Hash_RIGHT:
            return toc_RIGHT;
        case Hash_MOVE:
            return toc_MOVE;
        case Hash_ANGLE:
            return toc_ANGLE;
        case Hash_CIRCLE:
            return toc_CIRCLE;
        case Hash_CLEAR:
            return toc_CLEAR;
        case Hash_LABEL:
            return toc_LABEL;
        case Hash_GOTO:
            return toc_GOTO;
        case Hash_EVAL:
            return toc_EVAL;
        case Hash_PUSH:
            return toc_PUSH;
        case Hash_POP:
            return toc_POP;
        case Hash_PEEK:
            return toc_PEEK;
        case Hash_PUSHVEC:
            return toc_PUSHVEC;
        case Hash_POPVEC:
        case Hash_POP_VEC:
            return toc_POPVEC;
        case Hash_PEEKVEC:
            return toc_PEEKVEC;
        case Hash_IF:
            return toc_IF;
        case Hash_TURTLE:
            return toc_TURTLE;
        case Hash_INC:
            return toc_INC;
        case Hash_DEC:
            return toc_DEC;
        case Hash_ZERO:
            return toc_ZERO;
        case Hash_STO:
            return toc_STO;
        case Hash_GOSUB:
            return toc_GOSUB;
        case Hash_RET:
            return toc_RET;
        case Hash_STACK:
            return toc_STACK;
        case Hash_FADEOUT:
            return toc_FADEOUT;
        case Hash_FADEIN:
            return toc_FADEIN;
        case Hash_PALETTE:
            return toc_PALETTE;
        case Hash_STOP:
            return toc_STOP;
        case Hash_INIT:
            return toc_INIT;
        case Hash_RECT:
            return toc_RECT;
        case Hash_KEYSCAN:
            return toc_KEYSCAN;
        case Hash_KEYDOWN:
            return toc_KEYDOWN;
        case Hash_GETKEY:
            return toc_GETKEY;
        case Hash_IFKEYDOWN:
            return toc_IFKEYDOWN;
        case Hash_IFKEYUP:
            return toc_IFKEYUP;
        case Hash_FILL:
            return toc_FILL;
        case Hash_KEYUP:
            return toc_KEYUP;
        case Hash_TEXT:
            return toc_TEXT;
        case Hash_SPEED:
            return toc_SPEED;
        case Hash_DRAWSCREEN:
            return toc_DRAWSCREEN;
        case Hash_DRAWBUFFER:
            return toc_DRAWBUFFER;
        case Hash_SWAPDRAW:
            return toc_SWAPDRAW;
        case Hash_BLITSCREEN:
            return toc_BLITSCREEN;
        case Hash_BLITBUFFER:
            return toc_BLITBUFFER;
        case Hash_ELLIPSE:
            return toc_ELLIPSE;
        case Hash_SPRITE:
            return toc_SPRITE;
        case Hash_DEFSPRITE:
            return toc_DEFSPRITE;
        case Hash_SIZESPRITE:
            return toc_SIZESPRITE;
        case Hash_PALSHIFT:
            return toc_PALSHIFT;
        case Hash_WRAP:
            return toc_WRAP;
        case Hash_AUTODRAW:
            return toc_AUTODRAW;
        case Hash_DRAW:
            return toc_DRAW;
        case Hash_ONERROR:
            return toc_ONERROR;
        default:
            return toc_UNKNOWN;
    }
}

#ifdef __cplusplus
}
#endif

#endif