#include <sys/util.h>
#include <graphx.h>
#include <math.h>
#include <string.h>
#include <sys/rtc.h>
#include <stdlib.h>
#include <debug.h>
#include <fileioc.h>
#include <keypadc.h>

#include "turtle.h"
#include "palette.h"
#include "static.h"

#define NumTurtles 100
#define NumStacks 100
#define NewLineToken 0x3F
#define SpaceToken 0x29

extern real_t STATIC_REAL_360;
extern real_t STATIC_REAL_GFX_LCD_WIDTH;
extern real_t STATIC_REAL_GFX_LCD_HEIGHT;

size_t stream(const uint8_t* data, const size_t dataLength, const uint8_t delim, size_t* index)
{
    size_t start = *index;

    if (dataLength == 0)
    {
        return 0;
    }

    if (start > dataLength)
    {
        return 0;
    }

    do {
        char c = data[*index];
        *index = *index + 1;
        if (c == delim)
        {
            return *index - start; 
        }
    } while (*index < dataLength);
    
    *index = dataLength;
    return *index - start + 1;
}

// http://www.cse.yorku.ca/~oz/\nhash.html
uint32_t hash(const uint8_t* arr, size_t length)
{
    uint32_t hash = 5381;
    uint8_t c;

    while (length--)
    {
        c = *arr++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

#define hash_literal(str) hash(str, strlen(str))
Turtle turtles[NumTurtles];

int main(void)
{
    while(kb_AnyKey());

    const char* filename = "SQUARE";
    uint8_t programHandle = ti_OpenVar(filename, "r", OS_TYPE_PRGM);
    if (programHandle == 0)
    {
        return 1;
    }

    gfx_Begin();
    gfx_FillScreen(0);
    gfx_SetDrawBuffer();

    Static_Initialize();
    memset(turtles, 0, NumTurtles * sizeof(Turtle));
    Turtle_Initialize(turtles);

    size_t programSize = ti_GetSize(programHandle);
    uint8_t* program = malloc(programSize * sizeof(void*));
    ti_Read(program, programSize, 1, programHandle);
    
    srand(rtc_Time());
    bool exit = false;

    size_t index = 0;
    // prgmTURTLE:Return
    stream(program, programSize, NewLineToken, &index);
    // Comment
    uint8_t* comment = &program[index];
    size_t commentLength = stream(program, programSize, NewLineToken, &index);
    dbg_printf("prgm%s - \"%.*s\"\n", filename, commentLength, comment);

    uint8_t currentTurtleIndex = 0;

    dbg_printf("Program size: %d Cursor: %d\n", programSize, index);

    do  {
        kb_Scan();
    } while (!(kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter)));

    while (!exit)
    {
        Turtle* currentTurtle = &turtles[currentTurtleIndex];

        uint8_t* command = &program[index];
        size_t commandLength = stream(program, programSize, SpaceToken, &index) - 1;
        
        uint32_t commandHash = hash(command, commandLength);
        
        uint8_t* params = &program[index];
        size_t paramsLength = stream(program, programSize, NewLineToken, &index) - 1;
        
        dbg_printf("Command: \"%.*s\" (%d) Params: \"%.*s\" (%d) Hash: 0x%lx Cursor: %d\n", commandLength, command, commandLength, paramsLength, params, paramsLength, commandHash, index);
        if (os_Eval(params, paramsLength)) 
        {
            dbg_printf("\tFailed to eval \"%.*s\" length: %d \n", paramsLength, params, paramsLength);
            goto end_eval;
        }
        dbg_printf("\tEvaluation succeeded ");

        uint8_t type;
        void* ans = os_GetAnsData(&type);
        dbg_printf("type: %d\n", type);
        
        real_t param1 = STATIC_REAL_0;
        real_t param2 = STATIC_REAL_0;
        
        if (ans) 
        {
            list_t* ansList;
            cplx_list_t* cplx_ansList;
            switch (type)
            {
                case OS_TYPE_REAL:
                    memcpy(&param1, ans, sizeof(real_t));
                    break;
                case OS_TYPE_CPLX:
                    memcpy(&param1, ans, sizeof(real_t));
                    break;
                case OS_TYPE_REAL_LIST:
                    ansList = (list_t*)ans;
                    if (ansList->dim >= 1)
                        memcpy(&param1, ansList->items, sizeof(real_t));
                    if (ansList->dim >= 2)
                        memcpy(&param2, &ansList->items[1], sizeof(real_t));
                    break;
                case OS_TYPE_CPLX_LIST:
                    cplx_ansList = (cplx_list_t*)ans;
                    if (cplx_ansList->dim >= 1)
                        memcpy(&param1, cplx_ansList->items, sizeof(real_t));
                    if (cplx_ansList->dim >= 2)
                        memcpy(&param2, &cplx_ansList->items[1], sizeof(real_t));
                    break;
                case OS_TYPE_EQU:
                    break;
            }
        }

        switch (commandHash)
        {
            case HASH_COLOR:
                Turtle_SetColor(currentTurtle, &param1);
                break;
            case HASH_PEN:
                Turtle_SetPen(currentTurtle, &param1);
                break;
            case HASH_FORWARD:
                Turtle_Forward(currentTurtle, &param1);
                break;
            case HASH_LEFT:
                Turtle_Left(currentTurtle, &param1);
                break;
            case HASH_RIGHT:
                Turtle_Right(currentTurtle, &param1);
                break;
            case HASH_MOVE:
                Turtle_Goto(currentTurtle, &param1, &param2);
                break;
            case HASH_ANGLE:
                Turtle_SetAngle(currentTurtle, &param1);
                break;
            case HASH_CIRCLE:
                break;
            case HASH_CLEAR:
                break;
            case HASH_LABEL:
                break;
            case HASH_GOTO:
                break;
            case HASH_EVAL:
                break;
            case HASH_PUSH:
                break;
            case HASH_POP:
                break;
            case HASH_PEEK:
                break;
            case HASH_PUSHVEC:
                break;
            case HASH_POPVEC:
                break;
            case HASH_PEEKVEC:
                break;
            case HASH_IF:
                break;
        }
        dbg_printf("\tnow at index: %d programSize: %d >=? %d\n", index, programSize - 1, (index >= programSize - 1));
end_eval:
        if (index >= programSize - 1)
        {
            exit = true;
        }

        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);

        gfx_BlitScreen();

        for (int i = 0; i < NumTurtles; i++) {
            if (!turtles[i].initialized)
                continue;
            Turtle_Draw(&turtles[i]);
        }
        
        gfx_SwapDraw();
    }

    gfx_SetTextFGColor(124);
    gfx_SetTextBGColor(0);
    gfx_BlitScreen();
    gfx_PrintStringXY("Done", 1, 1);
    gfx_SwapDraw();

    do  {
        kb_Scan();
    } while (!(kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter)));

    ti_Close(programHandle);
    gfx_End();

    return 0;
}
