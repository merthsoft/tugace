#ifndef _STATIC_H_
#define _STATIC_H_

#include <keypadc.h>

#include <stdint.h>
#include <stdbool.h>

#include <ti/tokens.h>

#ifdef __cplusplus
extern "C" {
#endif

#define clear_key_buffer()                  while(kb_AnyKey())
#define null_coalesce(this, orThat)         (this ? this : orThat)
#define if_null_then_a_else_b(x, a, b)      (!x ? a : b)

#define hexCharToVal(c)                     ((c >= '0' && c <= '9') ? (c - '0') : (c - 'A' + 10))
#define convertHexPairToByte(buffer, i)     ((hexCharToVal(buffer[i]) << 4) | hexCharToVal(buffer[i+1]))

typedef uint24_t    LabelIndex;
typedef uint8_t     SpriteIndex;
typedef uint8_t     TurtleIndex;
typedef uint8_t     ProgramToken;
typedef uint8_t     StackIndex;
typedef uint8_t     StackPointer;
typedef size_t      ProgramCounter;

#define NumTurtles       10
// TODO: Can I make this 7 and it'll capture the sprite ints? Might be weird for the end-user
#define NumDataFields    6
#define NumStackPages    10
#define MaxStackDepth    256
#define NumLabels        1024
#define SystemStackDepth MaxStackDepth
#define NumSprites       256

#define Token_NoEvalParams    OS_TOK_DOUBLE_QUOTE
#define Token_EvalParams      OS_TOK_ADD

#define Token_Header_SpritePrefix OS_TOK_COLON
#define Token_Header_DescPrefix   OS_TOK_DOUBLE_QUOTE

#define Token_NewLine    OS_TOK_NEWLINE
#define Token_Space      OS_TOK_SPACE
#define Token_Comment    OS_TOK_DOUBLE_QUOTE
#define Token_Indent     OS_TOK_SPACE

#define Token_Label      OS_TOK_COLON
#define Token_Goto       OS_TOK_DECIMAL_POINT
#define Token_LabelOs    OS_TOK_LBL
#define Token_GotoOs     OS_TOK_GOTO

#define Token_If        OS_TOK_IF
#define Token_IfOs      OS_TOK_QUESTION

#define Token_StopOs    OS_TOK_STOP

#define Token_GoSub     OS_TOK_MULTIPLY
#define Token_Ret       OS_TOK_DIVIDE

#define Token_Inc       OS_TOK_ADD
#define Token_Dec       OS_TOK_SUB

#define Token_Forward  OS_TOK_POWER
#define Token_Left     OS_TOK_LEFT_PAREN
#define Token_LeftOs   OS_TOK_LEFT
#define Token_Right    OS_TOK_RIGHT_PAREN

#define Token_Push     OS_TOK_LEFT_BRACE
#define Token_Pop      OS_TOK_RIGHT_BRACE

#define Token_Turtle   OS_TOK_X

#define Token_Sto      OS_TOK_STO

#define Hash_COLOR      0xE809A4
#define Hash_PEN        0x881068
#define Hash_FORWARD    0x70717A
#define Hash_LEFT       0x87EB30
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
#define Hash_KEYSCAN    0x7C8B53
#define Hash_KEYDOWN    0x748786
#define Hash_GETKEY     0x8D3C4E
#define Hash_IFKEYDOWN  0x4DE395
#define Hash_FILL       0x84B2AC
#define Hash_PALSHIFT   0x1DFA20
#define Hash_TEXT       0x8C508A
#define Hash_WRAP       0x8E29FF
#define Hash_ONERROR    0xF9B72C
#define Hash_SPRITE     0x5111FC
#define Hash_SIZESPRITE 0xA06A97
#define Hash_DEFSPRITE  0xC7998B
#define Hash_AUTODRAW   0x2E172C
#define Hash_DRAWSCREEN 0x000005
#define Hash_DRAWBUFFER 0x3882ED
#define Hash_SWAPDRAW   0x839D0E
#define Hash_DRAW       0x83BED3

#define Hash_SCALE      0x000C00 


#define Hash_VGF2P8AFFINEINVQB 0x888888

#define Hash_PEER       0x111111

#define Hash_IFKEYUP    0x000009
#define Hash_KEYUP      0x000032

#define Hash_SPEED      0x000004

#define Hash_BLITSCREEN 0x000007
#define Hash_BLITBUFFER 0x000017

#define Hash_ELLIPSE    0x000008

#define Hash_ASM        0x0AAAA0

typedef enum TugaOpCode {
    toc_UNKNOWN,
    toc_NOP,
    toc_COLOR,
    toc_PEN,
    toc_WRAP,
    toc_FORWARD,
    toc_LEFT,
    toc_RIGHT,
    toc_MOVE,
    toc_ANGLE,
    toc_CIRCLE,
    toc_CLEAR,
    toc_LABEL,
    toc_GOTO,
    toc_EVAL,
    toc_PUSH,
    toc_POP,
    toc_PEEK,
    toc_PEER,
    toc_PUSHVEC,
    toc_POPVEC,
    toc_PEEKVEC,
    toc_IF,
    toc_TURTLE,
    toc_INC,
    toc_DEC,
    toc_ZERO,
    toc_STO,
    toc_GOSUB,
    toc_RET,
    toc_STACK,
    toc_FADEOUT,
    toc_FADEIN,
    toc_PALETTE,
    toc_STOP,
    toc_INIT,
    toc_RECT,
    toc_KEYSCAN,
    toc_KEYDOWN,
    toc_GETKEY,
    toc_IFKEYDOWN,
    toc_IFKEYUP,
    toc_FILL,
    toc_KEYUP,
    toc_TEXT,
    toc_SPEED,
    toc_DRAW,
    toc_AUTODRAW,
    toc_ONERROR,
    toc_DRAWSCREEN,
    toc_DRAWBUFFER,
    toc_SWAPDRAW,
    toc_BLITSCREEN,
    toc_BLITBUFFER,
    toc_ELLIPSE,
    toc_SIZESPRITE,
    toc_DEFSPRITE,
    toc_SPRITE,
    toc_SCALE,
    toc_PALSHIFT,
    toc_ASM,
} TugaOpCode;

#ifdef __cplusplus
}
#endif

#endif