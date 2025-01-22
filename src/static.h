#ifndef _CONST_H_
#define _CONST_H_

#include <ti/real.h>

extern real_t STATIC_REAL_0;
extern real_t STATIC_REAL_1;
extern real_t STATIC_REAL_255;
extern real_t STATIC_REAL_360;

extern real_t STATIC_REAL_GFX_LCD_WIDTH;
extern real_t STATIC_REAL_GFX_LCD_HEIGHT;

extern real_t STATIC_REAL_GFX_LCD_WIDTH_HALF;
extern real_t STATIC_REAL_GFX_LCD_HEIGHT_HALF;

#define HASH_COLOR    0xCE809A4
#define HASH_PEN      0xB881068
#define HASH_FORWARD  0xCE70717A
#define HASH_LEFT     0x7C87EB30
#define HASH_RIGHT    0xDF418C3
#define HASH_MOVE     0x7C88A41C
#define HASH_ANGLE    0xCC3368C
#define HASH_CIRCLE   0xA97FC1D7
#define HASH_CLEAR    0xCE644EC
#define HASH_LABEL    0xD830D05
#define HASH_GOTO     0x7C85599E
#define HASH_EVAL     0x7C845C2D
#define HASH_PUSH     0x7C8A6265
#define HASH_POP      0xB8811B4
#define HASH_PEEK     0x7C8A1C8A
#define HASH_PUSHVEC  0xDE3FFB43
#define HASH_POPVEC   0xC83E9C32
#define HASH_PEEKVEC  0xB7F1BF48
#define HASH_IF       0x5973F4
#define HASH_TURTLE   0xD200E2E5

void Static_Initialize(void);

#endif