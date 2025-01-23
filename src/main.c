#include <fileioc.h>
#include <keypadc.h>
#include <graphx.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/util.h>
#include <sys/rtc.h>

#include <ti/tokens.h>

#include <debug.h>

#include "turtle.h"
#include "palette.h"
#include "static.h"

#define NumTurtles  10
#define NumStacks   3
#define NumLabels   100

#define NewLineToken    OS_TOK_NEWLINE
#define SpaceToken      OS_TOK_SPACE
#define CommentToken    OS_TOK_DOUBLE_QUOTE

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
void debug_print_tokens(void* buffer, size_t length, size_t* stringLength)
{
    void** ptr = &buffer;
    uint8_t tokenLength;
    size_t tokenStringLength;
    uint8_t i = 0;
    if (stringLength)
        *stringLength = 0;
    while(i < length)
    {
        dbg_printf("%s", ti_GetTokenString(ptr, &tokenLength, &tokenStringLength));
        i += tokenLength;
        if (stringLength)
            *stringLength += tokenStringLength;
    }
}
#endif

#define null_coalesce(this, orThat) (this ? this : orThat)

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
    
    size_t programCounter = 0;
    // Header
    stream(program, programSize, NewLineToken, &programCounter);
    // Comment
    uint8_t* comment = &program[programCounter];
    size_t commentLength = stream(program, programSize, NewLineToken, &programCounter) - 1;
    size_t programStart = programCounter;
    
    #ifdef DEBUG
    dbg_printf("prgm%s - ", filename);
    debug_print_tokens(comment, commentLength, NULL);
    dbg_printf("\n");
    dbg_printf("Program size: %d PC: %d\n", programSize, programCounter);
    #endif
    
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[11] = "FPS: XX.XX";
    gfx_SetTextFGColor(124);
    
    uint8_t currentTurtleIndex = 0;
    bool exit = false;
    bool running = true;
    bool showFps = true;
    while (!exit)
    {
        if (!running)
            goto end_eval;
        Turtle* currentTurtle = &turtles[currentTurtleIndex];

        if (program[programCounter] == CommentToken)
        {
            dbg_printf("Skipping comment.");
            goto end_eval;
        }

        size_t startPc = programCounter;
        uint8_t* command = &program[programCounter];
        size_t commandLength = stream(program, programSize, SpaceToken, &programCounter) - 1;
        
        uint32_t commandHash = hash(command, commandLength);
        
        uint8_t* params = &program[programCounter];
        size_t paramsLength = stream(program, programSize, NewLineToken, &programCounter) - 1;
        
        #ifdef DEBUG
        size_t outputTokenStringLength;
        dbg_printf("%.4d: 0x%.8lX ", startPc, commandHash);
        debug_print_tokens(command, commandLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 10)
        {
            dbg_printf(" ");
            outputTokenStringLength++;
        }

        debug_print_tokens(params, paramsLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 30)
        {
            dbg_printf(" ");
            outputTokenStringLength++;
        }
        #endif
        
        char param1Var[4]; 
        char param2Var[4];
            
        real_t* param1 = NULL;
        real_t* param2 = NULL;

        int24_t param1Int;

        if (paramsLength > 0) 
        {
            if (os_Eval(params, paramsLength)) 
            {
                dbg_printf("\n\tSYNTAX ERROR: Failed to eval \"%.*s\" length: %d \n", paramsLength, params, paramsLength);
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
                        dbg_printf("\n\tSYNTAX ERROR: Equations not yet supported.");
                        goto end_eval;
                    default:
                        dbg_printf("\n\tSYNTAX ERROR: Unsupported ans type: %d.", type);
                        goto end_eval;
                }
            } else {
                dbg_printf("\n\tUNKNOWN ERROR: Failed to resolve ans.");
                goto end_eval;
            }
        }

        #ifdef DEBUG
        dbg_printf("\t");
        if (param1)
            dbg_printf("Param1: %f", os_RealToFloat(param1));
        else
            dbg_printf("Param1: NULL\t");
        dbg_printf("\t");
        if (param2)
            dbg_printf("Param2: %f", os_RealToFloat(param2));
        else
            dbg_printf("Param2: NULL\t");
        dbg_printf("\t");
        #endif

        int errNo;
        switch (commandHash)
        {
            case HASH_COLOR:
                Turtle_SetColor(currentTurtle, null_coalesce(param1, &STATIC_REAL_0));
                break;
            case HASH_PEN:
                Turtle_SetPen(currentTurtle, null_coalesce(param1, &STATIC_REAL_0));
                break;
            case HASH_FORWARD:
                Turtle_Forward(currentTurtle, null_coalesce(param1, &STATIC_REAL_1));
                break;
            case HASH_LEFT:
                Turtle_Left(currentTurtle, null_coalesce(param1, &STATIC_REAL_1));
                break;
            case HASH_RIGHT:
                Turtle_Right(currentTurtle, null_coalesce(param1, &STATIC_REAL_1));
                break;
            case HASH_MOVE:
                Turtle_Goto(currentTurtle, null_coalesce(param1, &STATIC_REAL_0), null_coalesce(param2, &STATIC_REAL_0));
                break;
            case HASH_ANGLE:
                Turtle_SetAngle(currentTurtle, null_coalesce(param1, &STATIC_REAL_0));
                break;
            case HASH_CIRCLE:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_CLEAR:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_LABEL:
                if (param1 == NULL)
                {
                    dbg_printf("\n\tSYNTAX ERROR: No label.");
                    break;
                }
                param1Int = os_RealToInt24(param1);
                if (param1Int >= 0 && param1Int < NumLabels)
                    labels[param1Int] = programCounter;
                break;
            case HASH_GOTO:
                if (param1 == NULL)
                {
                    dbg_printf("\n\tSYNTAX ERROR: No label.");
                    break;
                }
                param1Int = os_RealToInt24(param1);
                if (param1Int >= 0 && param1Int < NumLabels && labels[param1Int] >= programStart && labels[param1Int] < programSize)
                    programCounter = labels[param1Int];
                break;
            case HASH_EVAL:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_PUSH:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_POP:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_PEEK:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_PUSHVEC:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_POPVEC:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_PEEKVEC:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_IF:
                dbg_printf(" Unimplemented.");
                break;
            case HASH_ZERO:
                errNo = os_SetRealVar(param1Var, &STATIC_REAL_0);
                if (errNo)
                {
                    dbg_printf("\n\tSYNTAX ERROR: Got error trying to zero out %c: %d", param1Var[0], errNo);    
                }
                break;
            case HASH_INC:
                if (param1 == NULL)
                {
                    dbg_printf("\n\tSYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo)
                {
                    if (!param2)
                        param2 = &STATIC_REAL_1;
                    *param1 = os_RealAdd(param1, param2);
                    os_SetRealVar(param1Var, param1);
                }
                else {
                    dbg_printf("\n\tSYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            default:
                dbg_printf("\n\tSYNTAX ERROR: Unknown hash encountered");
                break;
        }
end_eval:
        #ifdef DEBUG
        if (running)
            dbg_printf("\n");
        #endif

        if (programCounter >= programSize - 1)
        {
            exit = true;
        }

        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);
        
        if (kb_IsDown(kb_KeyEnter))
        {
            running = !running;
            clear_key_buffer();
        }

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

        if (running)
        {
            if (showFps) 
            {
                gfx_SetColor(0);
                gfx_FillRectangle(0, 0, 84, 12);
                sprintf(buffer, "FPS: %.2f", fps);
                gfx_PrintStringXY(buffer, 2, 2);
            }
        }
        else
        {
            gfx_SetColor(0);
            gfx_FillRectangle(0, 0, 84, 12);
            gfx_PrintStringXY("Paused", 2, 2);
        }
        
        gfx_SwapDraw();
    }

    gfx_BlitScreen();
    gfx_SetTextFGColor(124);
    gfx_SetTextBGColor(0);
    gfx_SetColor(0);
    gfx_FillRectangle(0, 0, 84, 12);
    gfx_PrintStringXY("Done", 2, 2);
    gfx_SwapDraw();

    clear_key_buffer();
    do  {
        kb_Scan();
    } while (!(kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter)));

    ti_Close(programHandle);
    gfx_End();

    return 0;
}
