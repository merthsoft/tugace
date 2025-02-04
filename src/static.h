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

typedef char    ProgramToken;
typedef uint8_t LabelIndex;
typedef uint8_t StackIndex;
typedef uint8_t TurtleIndex;
typedef uint8_t StackPointer;
typedef size_t  ProgramCounter;

#define NumTurtles       10
#define NumDataFields    6
#define NumStacks        NumTurtles
#define MaxStackDepth    100
#define NumLabels        100
#define SystemStackDepth 255

#define NewLineToken    OS_TOK_NEWLINE
#define LineDelimToken  OS_TOK_COLON
#define SpaceToken      OS_TOK_SPACE
#define CommentToken    OS_TOK_DOUBLE_QUOTE

#define LabelToken      OS_TOK_COMMA
#define GotoToken       OS_TOK_DECIMAL_POINT
#define GosubToken      OS_TOK_MULTIPLY
#define RetToken        OS_TOK_DIVIDE

#define IncToken        OS_TOK_ADD
#define DecToken        OS_TOK_SUB

#define ForwardToken    OS_TOK_POWER
#define LeftToken       OS_TOK_LEFT_PAREN
#define RightToken      OS_TOK_RIGHT_PAREN

#define PushToken       OS_TOK_LEFT_BRACE
#define PopToken        OS_TOK_RIGHT_BRACE

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
#define HASH_POPVEC   0x4858FC // This is with the [PV] token lol
#define HASH_POP_VEC  0x3E9C32
#define HASH_PEEKVEC  0xF1BF48
#define HASH_IF       0x5973F4
#define HASH_TURTLE   0x00E2E5
#define HASH_INC      0x87F3bF
#define HASH_DEC      0x87DD51
#define HASH_ZERO     0x8F9A05
#define HASH_STO      0x881F1B
#define HASH_GOSUB    0x308A25
#define HASH_RET      0x8818F0
#define HASH_STACK    0x0C1F3B
#define HASH_FADEOUT  0xC13ECD
#define HASH_FADEIN   0xE6D28C
#define HASH_PALETTE  0x187774

void Static_Initialize(void);

#endif