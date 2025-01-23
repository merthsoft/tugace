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

#define clear_key_buffer() while(kb_AnyKey())

#ifdef DEBUG
void debug_print_tokens(void* buffer, size_t lenth)
{
    void** ptr = &buffer;
    for (uint8_t i = 0; i < lenth; i++)
    {
        dbg_printf("%s", ti_GetTokenString(ptr, NULL, NULL));
    }
}
#endif

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
    size_t commentLength = stream(program, programSize, NewLineToken, &programCounter) - 1;
    size_t programStart = programCounter;
    
    #ifdef DEBUG
    dbg_printf("prgm%s - ", filename);
    debug_print_tokens(comment, commentLength);
    dbg_printf("\n");
    dbg_printf("Program size: %d PC: %d\n", programSize, programCounter);
    #endif
    
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
        
        #ifdef DEBUG
        dbg_printf("Command: ");
        debug_print_tokens(command, commandLength);
        dbg_printf(" (%d)", commandLength);
        
        dbg_printf("\tParams: ");
        debug_print_tokens(params, paramsLength);
        dbg_printf(" (%d)", paramsLength);

        dbg_printf("\tHash: 0x%lx PC: %d", commandHash, programCounter);
        #endif
        
        real_t* param1 = NULL;
        real_t* param2 = NULL;
        char param1Var[4]; 
        char param2Var[4];

        int24_t param1Int;

        if (paramsLength > 0) 
        {
            if (os_Eval(params, paramsLength)) 
            {
                dbg_printf("\tFailed to eval \"%.*s\" length: %d \n", paramsLength, params, paramsLength);
                goto end_eval;
            }

            uint8_t type;
            void* ans = os_GetAnsData(&type);

            param1Var[0] = params[0];
            param1Var[1] = 0;
            param1Var[2] = 0;
            param1Var[3] = 0;

            param2Var[0] = 0;
            param2Var[1] = 0;
            param2Var[2] = 0;
            param2Var[3] = 0;
            
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
                dbg_printf("\tFailed to resolve ans.\n");
                goto end_eval;
            }
        }

        #ifdef DEBUG
        dbg_printf("\t");
        if (param1)
            dbg_printf("Param1: %f", os_RealToFloat(param1));
        else
            dbg_printf("Param1: NULL");
        dbg_printf("\t");
        if (param2)
            dbg_printf("Param2: %f", os_RealToFloat(param2));
        else
            dbg_printf("Param2: NULL");
        dbg_printf("\n");
        #endif

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
                Turtle_Goto(currentTurtle, param1, param2 ? param2 : &STATIC_REAL_0);
                break;
            case HASH_ANGLE:
                Turtle_SetAngle(currentTurtle, param1);
                break;
            case HASH_CIRCLE:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_CLEAR:
                dbg_printf("\tUnimplemented.\n");
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
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_PUSH:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_POP:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_PEEK:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_PUSHVEC:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_POPVEC:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_PEEKVEC:
                dbg_printf("\tUnimplemented.\n");
                break;
            case HASH_IF:
                dbg_printf("\tUnimplemented.\n");
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

    gfx_BlitScreen();
    gfx_SetTextFGColor(124);
    gfx_SetTextBGColor(0);
    gfx_SetColor(0);
    gfx_FillRectangle(0, 0, 84, 10);
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
