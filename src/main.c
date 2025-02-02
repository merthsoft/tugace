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

#ifdef DEBUG
void debug_print_tokens(const void* buffer, size_t length, size_t* stringLength)
{
    void** ptr = (void**)(&buffer);
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

size_t streamToNewline(const uint8_t* data, const size_t dataLength, const uint8_t delim, size_t* index)
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
        if (c == delim || c == NewLineToken)
        {
            return *index - start; 
        }
    } while (*index < dataLength);
    
    *index = dataLength;
    return *index - start + 1;
}

size_t labelSeek(const uint8_t* data, size_t dataLength, size_t dataStart, labelIndex label)
{
    if (dataLength == 0)
    {
        return 0;
    }
    
    size_t index = dataStart;
    while (index < dataLength) {
        do {
            char c = data[index];
            if (c == LabelToken)
            {
                break;
            }
            if (strncmp((const char*)data, "LABEL ", 6) == 0) {
                break;
            }
            streamToNewline(data, dataLength, NewLineToken, &index);
        } while (index < dataLength);
        
        if (index >= dataLength)
        {
            return 0;
        }

        index++;
        const uint8_t* params = &data[index];
        size_t paramsLength = streamToNewline(data, dataLength, NewLineToken, &index);

        if (paramsLength == 0)
        {
            continue;
            return 0;
        }
        
        if (os_Eval(params, paramsLength)) 
        {
            continue;
        }

        uint8_t type;
        void* ans = os_GetAnsData(&type);
        if (!ans)
        {
            continue;
        }
        real_t* realParam;
        uint24_t param;
        switch (type)
        {
            case OS_TYPE_REAL:
                realParam = ans;
                break;
            case OS_TYPE_CPLX:
                realParam = ans;
                break;
            default:
                continue;
        }
        param = os_RealToInt24(realParam);
        if (param == label)
            return index;
    }

    return 0;
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

#define null_coalesce(this, orThat) (this ? this : orThat)

static inline void pushTurtle(float* stack, uint8_t* stackPointer, Turtle* turtle)
{
    memcpy(&stack[*stackPointer], &turtle->x, sizeof(float)*NumDataFields);
    *stackPointer = *stackPointer + NumDataFields;
}

static inline void popTurtle(float* stack, uint8_t* stackPointer, Turtle* turtle)
{
    *stackPointer = *stackPointer - NumDataFields;
    memcpy(&turtle->x, &stack[*stackPointer], sizeof(float)*NumDataFields);
}

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
size_t labels[NumLabels];
uint8_t stackPointers[NumStacks];
float stacks[NumStacks][MaxStackDepth];

int main(void)
{
    clear_key_buffer();
    dbg_ClearConsole();

    const char* filename = "TREE";
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
    streamToNewline(program, programSize, NewLineToken, &programCounter);
    // Comment
    uint8_t* comment = &program[programCounter];
    size_t commentLength = streamToNewline(program, programSize, NewLineToken, &programCounter) - 1;
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
    
    turtleIndex currentTurtleIndex = 0;
    stackkIndex currentStackIndex = 0;

    bool exit = false;
    bool running = false;
    bool showFps = true;
    bool skipFlag = false;

    while (!exit)
    {
        if (!running)
            goto end_eval;

        Turtle* currentTurtle = &turtles[currentTurtleIndex];
        size_t startPc = programCounter;

        
        uint8_t* command = &program[programCounter];
        size_t commandLength = 0;
        uint32_t commandHash = 0;
        uint8_t* params;
        size_t paramsLength = 0;

        char shortHand = program[programCounter];
        switch (shortHand)
        {
            case CommentToken:
                dbg_printf("Skipping comment.");
                goto end_eval;
            case LabelToken:
                commandHash = HASH_LABEL;
                commandLength = 1;
                programCounter++;
                break;
            default:
                commandLength = streamToNewline(program, programSize, SpaceToken, &programCounter) - 1;
                commandHash = hash(command, commandLength);
                break;
        }
        
        if (program[programCounter-1] != NewLineToken) {
            params = &program[programCounter];
            paramsLength = streamToNewline(program, programSize, NewLineToken, &programCounter) - 1;
        }

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
        float eval;
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
                param1Int = (uint24_t)param1Val;
                gfx_FillScreen(param1Int % 256);
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
            case HASH_GOSUB:
            case HASH_GOTO:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= NumLabels)
                {
                    dbg_printf("SYNTAX ERROR: Invalid label: %d.", param1Int);
                    break;
                }
                size_t labelIndex = labels[param1Int];
                if (labelIndex <= programStart || labelIndex >= programSize)
                {
                    labelIndex = labelSeek(program, programSize, programStart, param1Int);
                    if (labelIndex <= programStart || labelIndex >= programSize)
                    {
                        dbg_printf("SYNTAX ERROR: Invalid label: %d - %d.", param1Int, labelIndex);
                        break;
                    }
                    labels[param1Int] = labelIndex;
                }
                if (commandHash == HASH_GOSUB)
                {
                    float pc = (float)programCounter;
                    push(stacks[currentStackIndex], &stackPointers[currentStackIndex], &pc);
                }
                dbg_printf(" found %d at %d ", param1Int, labelIndex);
                programCounter = labelIndex;
                dbg_printf(" jumping to %d ", programCounter);
                break;
            case HASH_EVAL:
                dbg_printf(" *");
                break;
            case HASH_PUSH:
                if (stackPointers[currentStackIndex] >= MaxStackDepth)
                {
                    dbg_printf("SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, stackPointers[currentStackIndex]);
                    break;
                }
                push(stacks[currentStackIndex], &stackPointers[currentStackIndex], &param1Val);
                break;
            case HASH_RET:
                if (stackPointers[currentStackIndex] == 0)
                {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                eval = pop(stacks[currentStackIndex], &stackPointers[currentStackIndex]);
                programCounter = (size_t)eval;
                break;
            case HASH_POP:
            case HASH_PEEK:
                if (commandHash == HASH_POP && stackPointers[currentStackIndex] == 0)
                {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                eval = pop(stacks[currentStackIndex], &stackPointers[currentStackIndex]);
                if (commandHash == HASH_PEEK) {
                    stackPointers[currentStackIndex]++;
                }
                retList[0] = eval;
                retListPointer = 1;
                break;
            case HASH_PUSHVEC:
                if (stackPointers[currentStackIndex] + 6 >= MaxStackDepth)
                {
                    dbg_printf("SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, stackPointers[currentStackIndex]);
                    break;
                }
                pushTurtle(stacks[currentStackIndex], &stackPointers[currentStackIndex], currentTurtle);
                break;
            case HASH_POPVEC:
            case HASH_PEEKVEC:
                if (commandHash == HASH_POPVEC && stackPointers[currentStackIndex] == NumDataFields-1)
                {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                popTurtle(stacks[currentStackIndex], &stackPointers[currentStackIndex], currentTurtle);
                if (commandHash == HASH_PEEKVEC) {
                    stackPointers[currentStackIndex] += NumDataFields;
                }
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
            case HASH_STO:
                if (param1 == NULL)
                {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo)
                {
                    if (!param2)
                    {
                        dbg_printf("SYNTAX ERROR: No value to set.");
                        break;
                    }
                    *param1 = os_RealCopy(param2);
                    os_SetRealVar(param1Var, param1);
                }
                else {
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            case HASH_TURTLE:
                dbg_printf(" *");
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

        //gfx_BlitScreen();

        for (int i = 0; i < NumTurtles; i++) {
            if (!turtles[i].initialized)
                continue;
            Turtle_Draw(&turtles[i]);
        }

        framecount++;
        if (clock() - time >= CLOCKS_PER_SEC)
        {
            fps = fps == 0 ? framecount : (fps + framecount) / 2.0f;
            framecount = 0;
            time = clock();
        }

        if (!running)
        {
            gfx_BlitScreen();
            gfx_SetTextBGColor(0);
            gfx_SetTextFGColor(124);
            gfx_PrintStringXY("Paused", 2, 2);
            do {
                kb_Scan();
            } while (!kb_IsDown(kb_KeyEnter));
            gfx_SwapDraw();
            gfx_SetDrawScreen();
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
