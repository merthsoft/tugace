#include <sys/util.h>
#include <graphx.h>
#include <math.h>
#include <string.h>
#include <sys/rtc.h>
#include <stdlib.h>
#include <fileioc.h>
#include <keypadc.h>
#include <time.h>

#include <debug.h>

#include "turtle.h"
#include "palette.h"
#include "static.h"

#define NumTurtles 10
#define NumStacks 3
#define NumLabels 100
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
static inline uint32_t hash(const uint8_t* arr, size_t length)
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

#define clear_key_buffer() while(kb_AnyKey())

int main(void)
{
    clear_key_buffer();

    Turtle turtles[NumTurtles];
    uint8_t labels[NumLabels];

    const char* filename = "SPIRAL2";
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

    size_t programCounter = 0;
    // Header
    stream(program, programSize, NewLineToken, &programCounter);
    // Comment
    uint8_t* comment = &program[programCounter];
    size_t commentLength = stream(program, programSize, NewLineToken, &programCounter);
    size_t programStart = programCounter;
    
    dbg_printf("prgm%s - \"%.*s\"\n", filename, commentLength, comment);
    dbg_printf("Program size: %d Cursor: %d\n", programSize, programCounter);
    
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[11] = "FPS: XX.XX";
    gfx_SetTextFGColor(124);
    
    uint8_t currentTurtleIndex = 0;
    while (!exit)
    {
        Turtle* currentTurtle = &turtles[currentTurtleIndex];

        uint8_t* command = &program[programCounter];
        size_t commandLength = stream(program, programSize, SpaceToken, &programCounter) - 1;
        
        uint32_t commandHash = hash(command, commandLength);
        
        uint8_t* params = &program[programCounter];
        size_t paramsLength = stream(program, programSize, NewLineToken, &programCounter) - 1;
        
        dbg_printf("Command: \"%.*s\" (%d) Params: \"%.*s\" (%d) Hash: 0x%lx Cursor: %d\n", commandLength, command, commandLength, paramsLength, params, paramsLength, commandHash, programCounter);
        
        if (os_Eval(params, paramsLength)) 
        {
            dbg_printf("\tFailed to eval \"%.*s\" length: %d \n", paramsLength, params, paramsLength);
            goto end_eval;
        }

        uint8_t type;
        void* ans = os_GetAnsData(&type);
        
        real_t* param1 = NULL;
        real_t* param2 = NULL;

        char param1Var[4]; 
        param1Var[0] = params[0];
        param1Var[1] = 0;
        param1Var[2] = 0;
        param1Var[3] = 0;
        char param2Var[4];
        param2Var[0] = 0;
        param2Var[1] = 0;
        param2Var[2] = 0;
        param2Var[3] = 0;

        int24_t param1Int;
        
        if (ans) 
        {
            list_t* ansList;
            cplx_list_t* cplx_ansList;
            switch (type)
            {
                case OS_TYPE_REAL:
                    param1 = ans;
                    break;
                case OS_TYPE_CPLX:
                    param1 = ans;
                    break;
                case OS_TYPE_REAL_LIST:
                    param1Var[0] = (char)*(params + 1);
                    param2Var[0] = (char)*(params + 3);
                    ansList = (list_t*)ans;
                    if (ansList->dim >= 1)
                        param1 = &ansList->items[0];
                    if (ansList->dim >= 2)
                        param2 = &ansList->items[1];
                    break;
                case OS_TYPE_CPLX_LIST:
                    param1Var[0] = (char)*(params + 1);
                    param2Var[0] = (char)*(params + 3);
                    cplx_ansList = (cplx_list_t*)ans;
                    if (cplx_ansList->dim >= 1)
                        param1 = &cplx_ansList->items[0].real;
                    if (cplx_ansList->dim >= 2)
                        param2 = &cplx_ansList->items[1].real;
                    break;
                case OS_TYPE_EQU:
                    break;
            }
        } else {
            dbg_printf("\tFailed to resolve ans.");
            goto end_eval;
        }
        int errNo;
        switch (commandHash)
        {
            case HASH_COLOR:
                Turtle_SetColor(currentTurtle, param1);
                break;
            case HASH_PEN:
                Turtle_SetPen(currentTurtle, param1);
                break;
            case HASH_FORWARD:
                Turtle_Forward(currentTurtle, param1);
                break;
            case HASH_LEFT:
                Turtle_Left(currentTurtle, param1);
                break;
            case HASH_RIGHT:
                Turtle_Right(currentTurtle, param1);
                break;
            case HASH_MOVE:
                Turtle_Goto(currentTurtle, param1, param2);
                break;
            case HASH_ANGLE:
                Turtle_SetAngle(currentTurtle, param1);
                break;
            case HASH_CIRCLE:
                break;
            case HASH_CLEAR:
                break;
            case HASH_LABEL:
                param1Int = os_RealToInt24(param1);
                if (param1Int >= 0 && param1Int < NumLabels)
                    labels[param1Int] = programCounter;
                break;
            case HASH_GOTO:
                param1Int = os_RealToInt24(param1);
                if (param1Int >= 0 && param1Int < NumLabels && labels[param1Int] >= programStart && labels[param1Int] < programSize)
                    programCounter = labels[param1Int];
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
            case HASH_ZERO:
                errNo = os_SetRealVar(param1Var, &STATIC_REAL_0);
                if (errNo)
                {
                    dbg_printf("\tGot error trying to zero out %c: %d\n", param1Var[0], errNo);    
                }
                break;
            case HASH_INC:
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo)
                {
                    param2 = &STATIC_REAL_1;
                    if (param2Var[0])
                    {
                        errNo = os_GetRealVar(param2Var, param2);
                        if (errNo)
                        {
                            dbg_printf("\tGot error trying to read %c: %d\n", param2Var[0], errNo);    
                            param2 = &STATIC_REAL_1;
                        }
                    }
                    *param1 = os_RealAdd(param1, param2);
                    os_SetRealVar(param1Var, param1);
                } 
                else {
                    dbg_printf("\tGot error trying to read %c: %d\n", param1Var[0], errNo);    
                }
                break;
            default:
                dbg_printf("\tUnknown hash encountered\n");
                break;
        }
end_eval:
        if (programCounter >= programSize - 1)
        {
            exit = true;
        }

        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);

        framecount++;
        if (clock() - time >= CLOCKS_PER_SEC)
        {
            fps = (fps + framecount) / 2.0f;
            framecount = 0;
            time = clock();
        }

        gfx_BlitScreen();

        for (int i = 0; i < NumTurtles; i++) {
            if (!turtles[i].initialized)
                continue;
            Turtle_Draw(&turtles[i]);
        }

        gfx_SetColor(0);
        gfx_FillRectangle(0, 0, 84, 10);
        sprintf(buffer, "FPS: %.2f", fps);
        gfx_PrintStringXY(buffer, 2, 2);
        
        gfx_SwapDraw();
    }

    gfx_SetTextFGColor(124);
    gfx_SetTextBGColor(0);
    gfx_BlitScreen();
    gfx_PrintStringXY("Done", 1, 1);
    gfx_SwapDraw();

    clear_key_buffer();
    do  {
        kb_Scan();
    } while (!(kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter)));

    ti_Close(programHandle);
    gfx_End();

    return 0;
}
