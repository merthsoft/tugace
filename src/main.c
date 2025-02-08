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

size_t streamToNewline(const ProgramToken* data, const size_t dataLength, ProgramToken additionalDelim, ProgramCounter* index) {
    const size_t start = *index;

    if (dataLength == 0) {
        return 0;
    }

    if (start > dataLength) {
        return 0;
    }

    const char* d = (char*)data;
    do {
        char c = d[*index];
        *index = *index + 1;
        if (c == NewLineToken || c == additionalDelim) {
            return *index - start; 
        }
    } while (*index < dataLength);
    
    *index = dataLength;
    return *index - start + 1;
}

ProgramCounter labelSeek(const ProgramToken* data, size_t dataLength, ProgramCounter dataStart, LabelIndex label) {
    if (dataLength == 0) {
        return 0;
    }
    const ProgramToken* d = (ProgramToken*)data;
    size_t index = dataStart;
    while (index < dataLength) {
        do {
            ProgramToken c = d[index];
            if (c == LabelToken || c == LabelTokenOS)
                break;
            if (strncmp((const char*)&d[index], "LABEL ", 6) == 0)
                break;
            
            streamToNewline(data, dataLength, NewLineToken, &index);
        } while (index < dataLength);
        
        if (index >= dataLength) {
            return 0;
        }

        index++;
        const ProgramToken* params = &d[index];
        size_t paramsLength = streamToNewline(d, dataLength, NewLineToken, &index);

        if (paramsLength == 0) {
            continue;
        }
        
        if (os_Eval(params, paramsLength)) {
            continue;
        }

        uint8_t type;
        void* ans = os_GetAnsData(&type);
        if (!ans) {
            continue;
        }
        real_t* realParam;
        uint24_t param;
        switch (type) {
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
static inline uint24_t hash(const ProgramToken* arr, size_t length) {
    uint32_t hash = 5381;
    uint8_t c;
    const uint8_t* d = (uint8_t*)arr;

    while (length--) {
        c = *d++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

#define clear_key_buffer() while(kb_AnyKey())

#define null_coalesce(this, orThat) (this ? this : orThat)

static inline void pushTurtle(float* stack, StackPointer* stackPointer, const Turtle* turtle) {
    memcpy(&stack[*stackPointer], &turtle->x, sizeof(float)*NumDataFields);
    *stackPointer = *stackPointer + NumDataFields;
}

static inline void popTurtle(float* stack, StackPointer* stackPointer, Turtle* turtle) {
    *stackPointer = *stackPointer - NumDataFields;
    memcpy(&turtle->x, &stack[*stackPointer], sizeof(float)*NumDataFields);
}

static inline void push(float* stack, StackPointer* stackPointer, float* value) {
    stack[*stackPointer] = *value;
    *stackPointer = *stackPointer + 1;
}

static inline float pop(float* stack, StackPointer* stackPointer) {
    *stackPointer = *stackPointer - 1;
    float ret = stack[*stackPointer];
    return ret;
}

Turtle turtles[NumTurtles];
ProgramCounter labels[NumLabels];
StackPointer systemStackPointer;
float systemStack[SystemStackDepth];
StackPointer stackPointers[NumStacks];
float stacks[NumStacks][MaxStackDepth];

uint16_t palette[256];

int main(void) {
    const char* filename = "SPIRALB";
    uint8_t programHandle = ti_OpenVar(filename, "r", OS_TYPE_PRGM);
    if (programHandle == 0) {
        return 1;
    }

    clear_key_buffer();
    dbg_ClearConsole();

    gfx_Begin();
    gfx_SetDrawScreen();
    //gfx_SetDrawBuffer();

    Static_Initialize();

    size_t programSize = ti_GetSize(programHandle);
    ProgramToken* program = malloc(programSize * sizeof(void*));
    ti_Read(program, programSize, 1, programHandle);
    
    srand(rtc_Time());
    
    ProgramCounter programCounter = 0;
    // Header
    streamToNewline(program, programSize, NewLineToken, &programCounter);
    // Comment
    ProgramToken* comment = &program[programCounter];
    size_t commentLength = streamToNewline(program, programSize, NewLineToken, &programCounter) - 1;
    ProgramCounter programStart = programCounter;
    
    #ifdef DEBUG
    dbg_printf("prgm%s - ", filename);
    debug_print_tokens(comment, commentLength, NULL);
    dbg_printf("\n");
    dbg_printf("Program size: %d PC: %d\n", programSize, programCounter);
    #endif
    
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[14] = "FPS: XXX     ";

program_start:
    memset(stackPointers, 0, sizeof(StackPointer)*NumStacks);
    systemStackPointer = 0;
    Turtle_Initialize(turtles);
    palette_default(palette);   
    gfx_SetPalette(palette, 256, 0);
    
    programCounter = programStart;

    TurtleIndex currentTurtleIndex = 0;
    StackIndex currentStackIndex = 0;

    bool exit = false;
    bool running = true;
    bool showFps = true;
    bool skipFlag = false;

    dbg_printf("Starting program exection.\n");
    while (!exit) {
        Turtle* currentTurtle = &turtles[currentTurtleIndex];
        if (!running)
            goto end_eval;

        ProgramCounter startPc = programCounter;

        ProgramToken* command = &program[programCounter];
        size_t commandLength = 0;
        uint32_t commandHash = 0;
        ProgramToken* params;
        size_t paramsLength = 0;

        ProgramToken shortHand = program[programCounter];
        switch (shortHand) {
            case CommentToken:
                dbg_printf("Skipping comment.");
                goto end_eval;
            case LabelToken:
            case GotoToken:
            case LabelTokenOS:
            case GotoTokenOS:
            case IfToken:
            case IfTokenOS:
                switch (shortHand) {
                    case LabelToken:
                    case LabelTokenOS:
                        commandHash = Hash_LABEL;
                        break;
                    case GotoToken:
                    case GotoTokenOS:
                        commandHash = Hash_GOTO;
                        break;
                    case IfToken:
                    case IfTokenOS:
                        commandHash = Hash_IF;
                        break;
                }
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
        dbg_printf("%.4d: ", startPc);
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

        if (skipFlag) {
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

        if (paramsLength > 0) {
            if (os_Eval(params, paramsLength)) {
                dbg_printf("SYNTAX ERROR: Failed to eval \"%.*s\" length: %d \n", paramsLength, (const char*)params, paramsLength);
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

            if (ans) {
                switch (type) {
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
            
            if (param1 != NULL) {
                param1Val = os_RealToFloat(param1);
                if (param2 != NULL)
                param2Val= os_RealToFloat(param2);
            }
        }

        #ifdef DEBUG
        if (param1) {
            dbg_printf("Param1: %f", os_RealToFloat(param1));
            dbg_printf("\t");
            if (param2)  {
                dbg_printf("Param2: %f", os_RealToFloat(param2));
                dbg_printf("\t");
            }
        }
        #endif

        int errNo;
        switch (commandHash) {
            case Hash_COLOR:
                Turtle_SetColor(currentTurtle, &param1Val);
                break;
            case Hash_PEN:
                Turtle_SetPen(currentTurtle, &param1Val);
                break;
            case Hash_FORWARD:
                Turtle_Forward(currentTurtle, &param1Val);
                break;
            case Hash_LEFT:
            case Hash_LEFT_OS:
                Turtle_Left(currentTurtle, &param1Val);
                break;
            case Hash_RIGHT:
                Turtle_Right(currentTurtle, &param1Val);
                break;
            case Hash_MOVE:
                Turtle_Goto(currentTurtle, &param1Val, &param2Val);
                break;
            case Hash_ANGLE:
                Turtle_SetAngle(currentTurtle, &param1Val);
                break;
            case Hash_CIRCLE:
                dbg_printf(" *");
                break;
            case Hash_CLEAR:
                param1Int = (uint24_t)param1Val;
                gfx_FillScreen(param1Int % 256);
                break;
            case Hash_LABEL:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= 0 && param1Int < NumLabels)
                    labels[param1Int] = programCounter;
                break;
            case Hash_GOSUB:
            case Hash_GOTO:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= NumLabels) {
                    dbg_printf("SYNTAX ERROR: Invalid label: %d.", param1Int);
                    break;
                }
                size_t labelIndex = labels[param1Int];
                if (labelIndex <= programStart || labelIndex >= programSize) {
                    labelIndex = labelSeek(program, programSize, programStart, param1Int);
                    if (labelIndex <= programStart || labelIndex >= programSize)
                    {
                        dbg_printf("SYNTAX ERROR: Invalid label: %d - %d.", param1Int, labelIndex);
                        break;
                    }
                    labels[param1Int] = labelIndex;
                }

                if (commandHash == Hash_GOSUB) {
                    if (systemStackPointer >= SystemStackDepth)
                    {
                        dbg_printf("SYNTAX ERROR: Max stack depth violated for stack system stack.");
                        break;
                    }
                    float pc = (float)programCounter;
                    push(systemStack, &systemStackPointer, &pc);
                }
                programCounter = labelIndex;
                break;
            case Hash_EVAL:
                dbg_printf(" *");
                break;
            case Hash_PUSH:
                if (stackPointers[currentStackIndex] >= MaxStackDepth) {
                    dbg_printf("SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, stackPointers[currentStackIndex]);
                    break;
                }
                push(stacks[currentStackIndex], &stackPointers[currentStackIndex], &param1Val);
                break;
            case Hash_RET:
                if (systemStackPointer == 0) {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for system stack.");
                    break;
                }
                eval = pop(systemStack, &systemStackPointer);
                programCounter = (size_t)eval;
                break;
            case Hash_POP:
            case Hash_PEEK:
                if (commandHash == Hash_POP && stackPointers[currentStackIndex] == 0) {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                eval = pop(stacks[currentStackIndex], &stackPointers[currentStackIndex]);
                if (commandHash == Hash_PEEK) {
                    stackPointers[currentStackIndex]++;
                }
                retList[0] = eval;
                retListPointer = 1;
                break;
            case Hash_PUSHVEC:
                if (stackPointers[currentStackIndex] + 6 >= MaxStackDepth) {
                    dbg_printf("SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, stackPointers[currentStackIndex]);
                    break;
                }
                pushTurtle(stacks[currentStackIndex], &stackPointers[currentStackIndex], currentTurtle);
                break;
            case Hash_POPVEC:
            case Hash_POP_VEC:
            case Hash_PEEKVEC:
                if (commandHash == Hash_POPVEC && stackPointers[currentStackIndex] <= NumDataFields-1) {
                    dbg_printf("SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                popTurtle(stacks[currentStackIndex], &stackPointers[currentStackIndex], currentTurtle);
                if (commandHash == Hash_PEEKVEC) {
                    stackPointers[currentStackIndex] += NumDataFields;
                }
                break;                
            case Hash_IF:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No predicate.");
                    break;
                }
                if (!param1Val) {
                    skipFlag = true;
                }
                break;
            case Hash_ZERO:
                errNo = os_SetRealVar(param1Var, &StaticReal_0);
                if (errNo) {
                    dbg_printf("SYNTAX ERROR: Got error trying to zero out %c: %d", param1Var[0], errNo);    
                }
                break;
            case Hash_INC:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2)
                        param2 = &StaticReal_1;
                    *param1 = os_RealAdd(param1, param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            case Hash_DEC:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2)
                        param2 = &StaticReal_1;
                    *param1 = os_RealSub(param1, param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            case Hash_STO:
                if (param1 == NULL) {
                    dbg_printf("SYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2) {
                        dbg_printf("SYNTAX ERROR: No value to set.");
                        break;
                    }
                    *param1 = os_RealCopy(param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("SYNTAX ERROR: Got error trying to read %c: %d", param1Var[0], errNo);    
                }
                break;
            case Hash_TURTLE:
                param1Int = (int24_t)param1Val;
                if (param1Int < -1 || param1Int > NumTurtles) {
                    dbg_printf("SYNTAX ERROR: Invalid turtle number %d", param1Int);
                    break;
                }
                currentTurtleIndex = param1Int;
                break;
            case Hash_STACK:
                param1Int = (int24_t)param1Val;
                if (param1Int < -1 || param1Int > NumStacks) {
                    dbg_printf("SYNTAX ERROR: Invalid stack number %d", param1Int);
                    break;
                }
                currentStackIndex = param1Int;
                break;
            case Hash_FADEOUT:
                param1Int = (int24_t)param1Val;
                fade_out(palette, 0, 255, param1Int);
                break;
            case Hash_FADEIN:
                param1Int = (int24_t)param1Val;
                fade_in(palette, 0, 255, param1Int);
                break;
            case Hash_PALETTE:
                param1Int = (int24_t)param1Val;
                switch (param1Int) {
                    case 0:
                        palette_default(palette);
                        break;
                    default:
                        dbg_printf("SYNTAX ERROR: Invalid palette %d", param1Int);
                        break;
                }
                gfx_SetPalette(palette, 256, 0);
                break;
            case Hash_FILL:
                gfx_FloodFill(currentTurtle->x, currentTurtle->y, currentTurtle->color);
                break;
            case Hash_BLITSCREEN:
                gfx_BlitScreen();
                break;
            case Hash_BLITBUFFER:
                gfx_BlitBuffer();
                break;
            case Hash_DRAWBUFFER:
                gfx_SetDrawBuffer();
                break;
            case Hash_DRAWSCREEN:
                gfx_SetDrawScreen();
                break;
            case Hash_SWAPDRAW:
                gfx_SwapDraw();
                break;
            case Hash_INIT:
                Turtle_Initialize(currentTurtle);
                break;
            default:
                dbg_printf("SYNTAX ERROR: Unknown hash encountered 0x%.6lX", commandHash);
                break;
        }
        
        if (retListPointer == 0)
            goto end_eval;

        if (retListPointer == 1) {
            real_t ans = os_FloatToReal(retList[0]);
            dbg_printf("Ans: %.2f", retList[0]);
            os_SetRealVar(OS_VAR_ANS, &ans);
        }

end_eval:
        #ifdef DEBUG
        if (running)
            dbg_printf("\n");
        #endif
        if (programCounter >= programSize - 1) {
            exit = true;
        }

        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);
        
        if (kb_IsDown(kb_KeyEnter)) {
            running = !running;
        }

        //gfx_BlitScreen();

        for (int i = 0; i < NumTurtles; i++) {
            Turtle* t = &turtles[i];
            if (!t->initialized)
                continue;
            
            Turtle_Draw(t);
        }

        framecount++;
        if (clock() - time >= CLOCKS_PER_SEC) {
            fps = fps == 0 ? framecount : (fps + framecount) / 2.0f;
            framecount = 0;
            time = clock();
        }

        if (!running) {
            gfx_BlitScreen();
            gfx_SetTextBGColor(0);
            gfx_SetTextFGColor(124);
            gfx_PrintStringXY("Paused", 2, 2);
            clear_key_buffer();
            do {
                kb_Scan();
                exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter);
            } while (!exit);
            if (kb_IsDown(kb_KeyEnter)) {
                exit = false;
                running = true;
            }
            clear_key_buffer();
            gfx_BlitBuffer();
        } else if (showFps) {
            gfx_SetTextBGColor(0);
            gfx_SetTextFGColor(124);
            sprintf(buffer, "FPS: %d   ", (uint8_t)fps);
            gfx_PrintStringXY(buffer, 2, 2);
        }
        
        //gfx_SwapDraw();
    }

    dbg_printf("Done. PC: %d\n", programCounter);
    gfx_BlitScreen();
    gfx_SetTextFGColor(124);
    gfx_SetTextBGColor(0);
    gfx_SetColor(0);
    gfx_FillRectangle(0, 0, 84, 12);
    gfx_PrintStringXY("Done", 2, 2);
    
    clear_key_buffer();
    do  {
        kb_Scan();
    } while (!(kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode) | kb_IsDown(kb_KeyEnter)));

    kb_Scan();
    if (kb_IsDown(kb_KeyEnter)) {
        clear_key_buffer();
        gfx_BlitBuffer();
        goto program_start;
    }

    ti_Close(programHandle);
    gfx_End();

    return 0;
}