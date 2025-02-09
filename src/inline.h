#ifndef __STACK_H_
#define __STACK_H_

#include <string.h>

#include "static.h"
#include "turtle.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void PushTurtle_Inline(float* stack, StackPointer* stackPointer, const Turtle* turtle) {
    memcpy(&stack[*stackPointer], &turtle->X, sizeof(float)*NumDataFields);
    *stackPointer = *stackPointer + NumDataFields;
}

static inline void PopTurtle_InLine(float* stack, StackPointer* stackPointer, Turtle* turtle) {
    *stackPointer = *stackPointer - NumDataFields;
    memcpy(&turtle->X, &stack[*stackPointer], sizeof(float)*NumDataFields);
}

static inline void Push_InLine(float* stack, StackPointer* stackPointer, const float* value) {
    stack[*stackPointer] = *value;
    *stackPointer = *stackPointer + 1;
}

static inline float Pop_InLine(float* stack, StackPointer* stackPointer) {
    *stackPointer = *stackPointer - 1;
    float ret = stack[*stackPointer];
    return ret;
}

// http://www.cse.yorku.ca/~oz/hash.html
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

#ifdef __cplusplus
}
#endif

#endif