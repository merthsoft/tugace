#include <stdint.h>
#include <stdbool.h>

#include <ti/tokens.h>

#ifndef _CONST_H_
#define _CONST_H_

typedef uint8_t ProgramToken;
typedef uint8_t LabelIndex;
typedef uint8_t StackIndex;
typedef uint8_t TurtleIndex;
typedef uint8_t StackPointer;
typedef size_t  ProgramCounter;

#define NumTurtles       10
#define NumDataFields    6
#define NumStacks        NumTurtles
#define MaxStackDepth    256
#define NumLabels        256
#define SystemStackDepth 256

#define NewLineToken    OS_TOK_NEWLINE
#define SpaceToken      OS_TOK_SPACE
#define CommentToken    OS_TOK_DOUBLE_QUOTE

#define LabelToken      OS_TOK_COLON
#define GotoToken       OS_TOK_DECIMAL_POINT
#define LabelTokenOS    OS_TOK_LBL
#define GotoTokenOS     OS_TOK_GOTO

#define IfTokenOS       OS_TOK_IF
#define IfToken         OS_TOK_QUESTION

#define StopTokenOs     OS_TOK_STOP

#define GosubToken      OS_TOK_MULTIPLY
#define RetToken        OS_TOK_DIVIDE

#define IncToken        OS_TOK_ADD
#define DecToken        OS_TOK_SUB

#define ForwardToken    OS_TOK_POWER
#define LeftToken       OS_TOK_LEFT_PAREN
#define RightToken      OS_TOK_RIGHT_PAREN

#define PushToken       OS_TOK_LEFT_BRACE
#define PopToken        OS_TOK_RIGHT_BRACE

#define Hash_COLOR      0xE809A4
#define Hash_PEN        0x881068
#define Hash_FORWARD    0x70717A
#define Hash_LEFT       0x87EB30
#define Hash_LEFT_OS    0x5989A6
#define Hash_RIGHT      0xF418C3
#define Hash_MOVE       0x88A41C
#define Hash_ANGLE      0xC3368C
#define Hash_CIRCLE     0x7FC1D7
#define Hash_CLEAR      0xE644EC
#define Hash_LABEL      0x830D05
#define Hash_GOTO       0x85599E
#define Hash_EVAL       0x845C2D
#define Hash_PUSH       0x8A6265
#define Hash_POP        0x8811B4
#define Hash_PEEK       0x8A1C8A
#define Hash_PUSHVEC    0x3FFB43
#define Hash_POPVEC     0x4858FC // This is with the [PV] token lol
#define Hash_POP_VEC    0x3E9C32
#define Hash_PEEKVEC    0xF1BF48
#define Hash_IF         0x5973F4
#define Hash_TURTLE     0x00E2E5
#define Hash_INC        0x87F3bF
#define Hash_DEC        0x87DD51
#define Hash_ZERO       0x8F9A05
#define Hash_STO        0x881F1B
#define Hash_GOSUB      0x308A25
#define Hash_RET        0x8818F0
#define Hash_STACK      0x0C1F3B
#define Hash_FADEOUT    0xC13ECD
#define Hash_FADEIN     0xE6D28C
#define Hash_PALETTE    0x187774
#define Hash_STOP       0x8C02CB
#define Hash_INIT       0x866CB9
#define Hash_RECT       0x8B3513

#define Hash_FILL       0x000001

#define Hash_GETKEY     0x000002
#define Hash_KEYSCAN    0x000012
#define Hash_ISDOWN     0x000022
#define Hash_ISUP       0x000032

#define Hash_TEXT       0x000003
#define Hash_SPEED      0x000004

#define Hash_DRAWSCREEN 0x000005
#define Hash_DRAWBUFFER 0x000015
#define Hash_SWAPDRAW   0x000006
#define Hash_BLITSCREEN 0x000007
#define Hash_BLITBUFFER 0x000017
#define Hash_ELLIPSE    0x000008
#define Hash_SPRITE     0x000009
#define Hash_DEFSPRITE  0x00000A

#endif