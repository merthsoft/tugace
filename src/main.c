#include <fileioc.h>
#include <keypadc.h>
#include <graphx.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/util.h>
#include <sys/rtc.h>

#include <ti/real.h>
#include <ti/tokens.h>

#include <debug.h>

#include "turtle.h"
#include "palette.h"
#include "static.h"

#define NumStacks   NumTurtles
#define MaxStackDepth   100
#define NumLabels       100

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

// http://www.cse.yorku.ca/~oz/hash.html
static inline uint24_t hash(const uint8_t* arr, size_t length)
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

static inline void push(float* stack, uint8_t* stackPointer, float* value)
{
    stack[*stackPointer] = *value;
    *stackPointer = *stackPointer + 1;
}

static inline float pop(float* stack, uint8_t* stackPointer)
{
    *stackPointer = *stackPointer - 1;
    float ret = stack[*stackPointer];
    return ret;
}

Turtle turtles[NumTurtles];
uint8_t labels[NumLabels];
uint8_t stackPointers[NumStacks];
float stacks[NumStacks][MaxStackDepth];

int main(void)
{
    clear_key_buffer();
    dbg_ClearConsole();

    const char* filename = "SPIRAL2";
    uint8_t programHandle = ti_OpenVar(filename, "r", OS_TYPE_PRGM);
    if (programHandle == 0)
    {
        return 1;
    }

    gfx_Begin();
    gfx_SetDrawScreen();
    gfx_FillScreen(0);
    //gfx_SetDrawBuffer();

    Static_Initialize();
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
    
    uint8_t currentTurtleIndex = 0;
    bool exit = false;
    bool running = true;
    bool showFps = false;
    bool skipFlag = false;

    while (!exit)
    {
        if (!running)
            goto end_eval;

        Turtle* currentTurtle = &turtles[currentTurtleIndex];
        size_t startPc = programCounter;
        uint8_t* command = &program[programCounter];
        size_t commandLength = stream(program, programSize, SpaceToken, &programCounter) - 1;
        
        uint32_t commandHash = hash(command, commandLength);
        
        uint8_t* params = &program[programCounter];
        size_t paramsLength = stream(program, programSize, NewLineToken, &programCounter) - 1;
        
        #ifdef DEBUG
        size_t outputTokenStringLength;
        dbg_printf("%.4d: 0x%.6lX ", startPc, commandHash);
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


        if (program[programCounter] == CommentToken)
        {
            dbg_printf("Skipping comment.");
            goto end_eval;
        }

        if (skipFlag)
        {
            dbg_printf("Skipping because skipFlag is set.");
            skipFlag = false;
            goto end_eval;
        }
        
        real_t* param1 = NULL;
        real_t* param2 = NULL;
        float param1Val;
        float param2Val;
        int24_t param1Int;
        char param1Var[4];
        
        uint8_t type;
        void* ans;
        list_t* ansList;
        cplx_list_t* cplx_ansList;
        
        float retList[MaxStackDepth];
        uint8_t retListPointer = 0;

        if (paramsLength > 0) 
        {
            if (os_Eval(params, paramsLength)) 
            {
                dbg_printf("SYNTAX ERROR: Failed to eval \"%.*s\" length: %d \n", paramsLength, params, paramsLength);
                goto end_eval;
            }

            ans = os_GetAnsData(&type);

            param1Var[0] = params[0];
            param1Var[1] = 0;
            param1Var[2] = 0;
            param1Var[3] = 0;
            param1Val = 0;
            param2Val = 0;
            param1 = NULL;
            param2 = NULL;
            retListPointer = 0;

            if (ans) 
            {
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
                        ansList = (list_t*)ans;
                        if (ansList->dim >= 1)
                            param1 = &ansList->items[0];
                        if (ansList->dim >= 2)
                            param2 = &ansList->items[1];
                        break;
                    case OS_TYPE_CPLX_LIST:
                        param1Var[0] = (char)*(params + 1);
                        cplx_ansList = (cplx_list_t*)ans;
                        if (cplx_ansList->dim >= 1)
                            param1 = &cplx_ansList->items[0].real;
                        if (cplx_ansList->dim >= 2)
                            param2 = &cplx_ansList->items[1].real;
                        break;
                    case OS_TYPE_EQU:
                        dbg_printf("SYNTAX ERROR: Equations not yet supported.");
                        goto end_eval;
                    default:
                        dbg_printf("SYNTAX ERROR: Unsupported ans type: %d.", type);
                        goto end_eval;
                }
            } else {
                dbg_printf("UNKNOWN ERROR: Failed to resolve ans.");
                goto end_eval;
            }
            
            if (param1 != NULL)
            {
                param1Val = os_RealToFloat(param1);
                if (param2 != NULL)
                param2Val= os_RealToFloat(param2);
            }
        }

        #ifdef DEBUG
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
                Turtle_SetColor(currentTurtle, &param1Val);
                break;
            case HASH_PEN:
                Turtle_SetPen(currentTurtle, &param1Val);
                break;
            case HASH_FORWARD:
                Turtle_Forward(currentTurtle, &param1Val);
                break;
            case HASH_LEFT:
                Turtle_Left(currentTurtle, &param1Val);
                break;
            case HASH_RIGHT:
                Turtle_Right(currentTurtle, &param1Val);
                break;
            case HASH_MOVE:
                Turtle_Goto(currentTurtle, &param1Val, &param2Val);
                break;
            case HASH_ANGLE:
                Turtle_SetAngle(currentTurtle, &param1Val);
                break;
            case HASH_CIRCLE:
                dbg_printf(" *");
                break;
            case HASH_CLEAR:
                dbg_printf(" *");
                break;
            case HASH_LABEL:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= 0 && param1Int < NumLabels)
                    labels[param1Int] = programCounter;
                break;
            case HASH_GOTO:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= 0 && param1Int < NumLabels && labels[param1Int] >= programStart && labels[param1Int] < programSize)
                    programCounter = labels[param1Int];
                break;
            case HASH_EVAL:
                dbg_printf(" *");
                break;
            case HASH_PUSH:
                param1Int = (uint24_t)param1Val;
                if (param1Int > NumStacks)
                {
                    dbg_printf("SYNTAX ERROR: Invalid stack number %d.", param1Int);
                    break;
                }
                if (stackPointers[param1Int] >= MaxStackDepth)
                {
                    dbg_printf("SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", param1Int, stackPointers[param1Int]);
                    break;
                }
                push(stacks[param1Int], &stackPointers[param1Int], &param2Val);
                break;
            case HASH_POP:
            case HASH_PEEK:
                param1Int = (uint24_t)param1Val;
                if (param1Int > NumStacks)
                {
                    dbg_printf("SYNTAX ERROR: Invalid stack number %d.", param1Int);
                    break;
                }
                if (commandHash == HASH_POP && stackPointers[param1Int] == 0)
                {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", param1Int);
                    break;
                }
                float eval = pop(stacks[param1Int], &stackPointers[param1Int]);
                if (commandHash == HASH_PEEK) {
                    stackPointers[param1Int]++;
                }
                retList[0] = eval;
                retListPointer = 1;
                break;
            case HASH_PUSHVEC:
                dbg_printf(" *");
                break;
            case HASH_POPVEC:
                dbg_printf(" *");
                break;
            case HASH_PEEKVEC:
                dbg_printf(" *");
                break;
            case HASH_IF:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No predicate.");
                    break;
                }
                if (!param1Val) {
                    skipFlag = true;
                }
                break;
            case HASH_ZERO:
                errNo = os_SetRealVar(param1Var, &STATIC_REAL_0);
                if (errNo)
                {
                    dbg_printf("SYNTAX ERROR: Got error trying to zero out %c: %d", param1Var[0], errNo);    
                }
                break;
            case HASH_INC:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
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
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            case HASH_DEC:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo)
                {
                    if (!param2)
                        param2 = &STATIC_REAL_1;
                    *param1 = os_RealSub(param1, param2);
                    os_SetRealVar(param1Var, param1);
                }
                else {
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            default:
                dbg_printf("SYNTAX ERROR: Unknown hash encountered");
                break;
        }

        if (retListPointer == 0)
            goto end_eval;

        if (retListPointer == 1)
        {
            real_t ans = os_FloatToReal(retList[0]);
            dbg_printf(" setting ans to %.2f ", retList[0]);
            os_SetRealVar(OS_VAR_ANS, &ans);
        }

end_eval:
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
        #ifdef DEBUG
        if (running)
            dbg_printf("\tFPS: %.2f\n", fps);
        #endif

        //gfx_BlitScreen();

        for (int i = 0; i < NumTurtles; i++) {
            if (!turtles[i].initialized)
                continue;
            Turtle_Draw(&turtles[i]);
        }

        if (!running)
        {
            gfx_SetTextBGColor(0);
            gfx_SetTextFGColor(124);
            gfx_PrintStringXY("Paused", 2, 2);
        }
        else if (showFps) 
        {
            gfx_SetTextBGColor(0);
            gfx_SetTextFGColor(124);
            sprintf(buffer, "FPS: %.2f", fps);
            gfx_PrintStringXY(buffer, 2, 2);
        }
        
        //gfx_SwapDraw();
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
