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

#define HASH_COLOR    0xE809A4
#define HASH_PEN      0x881068
#define HASH_FORWARD  0x70717A
#define HASH_LEFT     0x87EB30
#define HASH_RIGHT    0xF418C3
#define HASH_MOVE     0x88A41C
#define HASH_ANGLE    0xC3368C
#define HASH_CIRCLE   0x7FC1D7
#define HASH_CLEAR    0xE644EC
#define HASH_LABEL    0x830D05
#define HASH_GOTO     0x85599E
#define HASH_EVAL     0x845C2D
#define HASH_PUSH     0x8A6265
#define HASH_POP      0x8811B4
#define HASH_PEEK     0x8A1C8A
#define HASH_PUSHVEC  0x3FFB43
#define HASH_POPVEC   0x3E9C32
#define HASH_PEEKVEC  0xF1BF48
#define HASH_IF       0x5973F4
#define HASH_TURTLE   0x00E2E5
#define HASH_INC      0x87F3bF
#define HASH_DEC      0x000001
#define HASH_ZERO     0x8F9A05

void Static_Initialize(void);

#endif