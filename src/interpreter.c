#include <graphx.h>
#include <time.h>

#include <sys/rtc.h>

#include <ti/real.h>
#include <ti/vars.h>

#include "const.h"
#include "keyhelper.h"
#include "inline.h"
#include "interpreter.h"
#include "palette.h"
#include "seek.h"
#include "static.h"
#include "turtle.h"

#ifdef DEBUG
//#define DEBUG_PROCESSOR
#include <debug.h>
#include <fileioc.h>

void debug_print_tokens(const void* buffer, size_t length, size_t* stringLength)
{
    uint8_t tokenLength = 0;
    size_t tokenStringLength = 0;
    uint8_t i = 0;
    if (stringLength)
        *stringLength = 0;
    
    while(i < length)
    {
        dbg_printf("%s", ti_GetTokenString((void**)&buffer, &tokenLength, &tokenStringLength));
        i += tokenLength;
        if (stringLength)
            *stringLength += tokenStringLength;
    }
}
#endif

// should be treated as private
Turtle Interpreter_turtles[NumTurtles];
ProgramCounter Interpreter_labels[NumLabels];
StackPointer Interpreter_stackPointers[NumStackPages];
StackPointer Interpreter_systemStackPointer;
float Interpreter_stacks[NumStackPages][MaxStackDepth];
float Interpreter_systemStack[SystemStackDepth];
uint16_t Interpreter_paletteBuffer[256];
gfx_sprite_t* Interpreter_spriteDictionary[NumSprites];

void Interpreter_Interpret(ProgramToken* program, size_t programSize) {
    gfx_Begin();
    srand(rtc_Time());
    
    ProgramCounter programCounter = 0;
    // Header
    Seek_ToNewLine(program, programSize, NewLineToken, &programCounter);
    // Comment
    ProgramToken* comment = &program[programCounter];
    size_t commentLength = Seek_ToNewLine(program, programSize, NewLineToken, &programCounter) - 1;
    ProgramCounter programStart = programCounter;
    
    #ifdef DEBUG
    debug_print_tokens(comment, commentLength, NULL);
    dbg_printf("\n");
    dbg_printf("Program size: %d PC: %d\n", programSize, programCounter);
    dbg_printf("size turtle: %d program: %p pc %p turtles: %p system stack: %p system sp: %p stacks: %p sp %p labels: %p palette: %p \n", sizeof(Turtle), program, &programCounter, Interpreter_turtles, Interpreter_systemStack, &Interpreter_systemStackPointer, Interpreter_stacks, Interpreter_stackPointers, Interpreter_labels, Interpreter_paletteBuffer);
    #endif
    
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[14] = "FPS: XXX     ";

    memset(Interpreter_spriteDictionary, 0, sizeof(gfx_sprite_t*)*NumSprites);
program_start:
    gfx_SetDrawScreen();

    memset(Interpreter_labels, 0, sizeof(ProgramCounter)*NumLabels);
    Interpreter_labels[255] = 0xABCDEF;
    memset(Interpreter_systemStack, 0, sizeof(float)*SystemStackDepth);
    memset(Interpreter_stackPointers, 0, sizeof(StackPointer)*NumStackPages);
    memset(Interpreter_stacks, 0, sizeof(float)*NumStackPages*MaxStackDepth);
    Interpreter_systemStackPointer = 0;
    Turtle_Initialize(Interpreter_turtles);
    Palette_Default(Interpreter_paletteBuffer); 
    #ifdef DEBUG
    Interpreter_paletteBuffer[1] = gfx_RGBTo1555(0, 125, 0);
    #endif  
    gfx_SetPalette(Interpreter_paletteBuffer, 256, 0);
    
    programCounter = programStart;

    TurtleIndex currentTurtleIndex = 0;
    StackIndex currentStackIndex = 0;

    bool exit = false;
    bool running = true;
    bool showFps = true;
    bool skipFlag = false;

    dbg_printf("Starting program exection.\n");
    while (!exit) {
        Turtle* currentTurtle = &Interpreter_turtles[currentTurtleIndex];
        if (!running)
            goto end_eval;
        
        #ifdef DEBUG_PROCESSOR
        dbg_printf("%.4d: ", programCounter);
        #endif

        ProgramToken* command = &program[programCounter];
        size_t commandLength = 0;
        uint32_t commandHash = 0;
        ProgramToken* params;
        size_t paramsLength = 0;

        ProgramToken shortHand = program[programCounter];
        switch (shortHand) {
            case CommentToken:
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("Skipping comment.");
                #endif
                goto end_eval;
            case LabelToken:
            case GotoToken:
            case LabelTokenOS:
            case GotoTokenOS:
            case IfToken:
            case IfTokenOS:
            case GosubToken:
            case StopTokenOs:
            case PushToken:
            case PopToken:
            case TurtleToken:
            case StoToken:
                switch (shortHand) {
                    case StoToken:
                        commandHash = Hash_STO;
                        break;
                    case TurtleToken:
                        commandHash = Hash_TURTLE;
                        break;
                    case PushToken:
                        commandHash = Hash_PUSH;
                        break;
                    case PopToken:
                        commandHash = Hash_POP;
                        break;
                    case StopTokenOs:
                        commandHash = Hash_STOP;
                        break;
                    case GosubToken:
                        commandHash = Hash_GOSUB;
                        break;
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
                commandLength = Seek_ToNewLine(program, programSize, SpaceToken, &programCounter) - 1;
                commandHash = Hash_InLine(command, commandLength);
                break;
        }
        
        if (program[programCounter-1] != NewLineToken) {
            params = &program[programCounter];
            paramsLength = Seek_ToNewLine(program, programSize, NewLineToken, &programCounter) - 1;
        }

        #ifdef DEBUG_PROCESSOR
        size_t outputTokenStringLength;
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
            #ifdef DEBUG_PROCESSOR
                dbg_printf("Skipping because skipFlag is set.");
            #endif
            skipFlag = false;
            goto end_eval;
        }
        
        real_t* param1 = NULL;
        real_t* param2 = NULL;
        float param1Val;
        float param2Val;
        int24_t param1Int;
        int24_t param2Int;
        float eval;
        int24_t intEval;
        char param1Var[4];
        
        uint8_t type;
        void* ans;
        list_t* paramsList;
        cplx_list_t* paramsListCplx;
        
        float retList[MaxStackDepth];
        uint16_t retListPointer = 0;

        if (paramsLength > 0) {
            if (os_Eval(params, paramsLength)) {
                dbg_printf("\nSYNTAX ERROR: Failed to eval \"%.*s\" length: %d.", paramsLength, (const char*)params, paramsLength);
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
            paramsList = NULL;
            paramsListCplx = NULL;
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
                        paramsList = (list_t*)ans;
                        if (paramsList->dim >= 1)
                            param1 = &paramsList->items[0];
                        if (paramsList->dim >= 2)
                            param2 = &paramsList->items[1];
                        break;
                    case OS_TYPE_CPLX_LIST:
                        param1Var[0] = (char)*(params + 1);
                        paramsListCplx = (cplx_list_t*)ans;
                        if (paramsListCplx->dim >= 1)
                            param1 = &paramsListCplx->items[0].real;
                        if (paramsListCplx->dim >= 2)
                            param2 = &paramsListCplx->items[1].real;
                        break;
                    case OS_TYPE_EQU:
                        dbg_printf("\nSYNTAX ERROR: Equations not yet supported.");
                        goto end_eval;
                    default:
                        dbg_printf("\nSYNTAX ERROR: Unsupported ans type: %d.", type);
                        goto end_eval;
                }
            } else {
                dbg_printf("\nUNKNOWN ERROR: Failed to resolve ans.");
                goto end_eval;
            }
            
            if (param1 != NULL) {
                param1Val = os_RealToFloat(param1);
                if (param2 != NULL)
                    param2Val = os_RealToFloat(param2);
            }
        }

        #ifdef DEBUG_PROCESSOR
        if (param1) {
            dbg_printf("Param1: %f", param1Val);
            dbg_printf("\t");
            if (param2)  {
                dbg_printf("Param2: %f", param2Val);
                dbg_printf("\t");
            }
        }
        #endif

        gfx_SetColor(currentTurtle->Color);
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
                param1Int = if_null_then_a_else_b(param1, 1, param1Val);
                if (currentTurtle->Pen)
                    gfx_FillCircle(currentTurtle->X, currentTurtle->Y, param1Int);
                else
                    gfx_Circle(currentTurtle->X, currentTurtle->Y, param1Int);
                break;
            case Hash_RECT:
                param1Int = if_null_then_a_else_b(param1, 1, param1Val);
                param2Int = if_null_then_a_else_b(param2, 1, param2Val);
                if (currentTurtle->Pen)
                    gfx_FillRectangle(currentTurtle->X, currentTurtle->Y, param1Int, param2Int);
                else
                    gfx_Rectangle(currentTurtle->X, currentTurtle->Y, param1Int, param2Int);
                break;
            case Hash_STOP:
                exit = true;
                break;
            case Hash_CLEAR:
                param1Int = (uint24_t)param1Val;
                gfx_FillScreen(param1Int % 256);
                break;
            case Hash_LABEL:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= 0 && param1Int < NumLabels)
                    Interpreter_labels[param1Int] = programCounter;
                break;
            case Hash_GOSUB:
            case Hash_GOTO:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No label.");
                    break;
                }
                param1Int = (uint24_t)param1Val;
                if (param1Int >= NumLabels) {
                    dbg_printf("\nSYNTAX ERROR: Invalid label: %d.", param1Int);
                    break;
                }
                size_t labelIndex = Interpreter_labels[param1Int];
                if (labelIndex <= programStart || labelIndex >= programSize) {
                    labelIndex = Seek_ToLabel(program, programSize, programStart, param1Int);
                    if (labelIndex <= programStart || labelIndex >= programSize)
                    {
                        dbg_printf("\nSYNTAX ERROR: Invalid label: %d - %d.", param1Int, labelIndex);
                        break;
                    }
                    Interpreter_labels[param1Int] = labelIndex;
                }

                if (commandHash == Hash_GOSUB) {
                    float pc = (float)programCounter;
                    Push_InLine(Interpreter_systemStack, &Interpreter_systemStackPointer, &pc);
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("pushing PC onto stack %f", pc);
                    #endif
                }
                programCounter = labelIndex;
                break;
            case Hash_EVAL:
                dbg_printf(" *");
                break;
            case Hash_PUSH:
                if (paramsList == NULL) {
                    if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                        dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                        goto end_eval;
                    }
                    Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &param1Val);
                } else {
                    for (uint16_t pushListIndex = 0; pushListIndex < paramsList->dim; pushListIndex++)  {
                        if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                            dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                            goto end_eval;
                        }
                        param1Val = os_RealToFloat(&paramsList->items[pushListIndex]);
                        Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &param1Val);
                    }
                }
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Main_stackPointers[currentStackIndex]);
                #endif
                break;
            case Hash_RET:
                if (Interpreter_systemStackPointer == 0) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for system stack.");
                    break;
                }
                eval = Pop_InLine(Interpreter_systemStack, &Interpreter_systemStackPointer);
                programCounter = (size_t)eval;
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("setting PC from stack %f pc %d", eval, programCounter);
                #endif
                break;
            case Hash_POP:
            case Hash_PEEK:
                if (commandHash == Hash_POP && Interpreter_stackPointers[currentStackIndex] == 0) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                eval = Pop_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex]);
                if (commandHash == Hash_PEEK) {
                    Interpreter_stackPointers[currentStackIndex]++;
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("peeked: %f", eval);
                        dbg_printf(" sp: %d", Main_stackPointers[currentStackIndex]);
                    #endif
                } else {
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("popped: %f", eval);
                        dbg_printf(" sp: %d", Main_stackPointers[currentStackIndex]);
                    #endif
                }
                if (param1 == NULL) {
                    retList[0] = eval;
                    retListPointer++;
                } else {
                    errNo = os_GetRealVar(param1Var, param1);
                    if (!errNo) {
                        *param1 = os_FloatToReal(eval);
                        os_SetRealVar(param1Var, param1);
                    } else {
                        dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);    
                    }
                }
                break;
            case Hash_PUSHVEC:
                if (Interpreter_stackPointers[currentStackIndex] + 6 >= MaxStackDepth) {
                    dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                    break;
                }
                PushTurtle_Inline(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Main_stackPointers[currentStackIndex]);
                #endif
                break;
            case Hash_POPVEC:
            case Hash_POP_VEC:
            case Hash_PEEKVEC:
                if (commandHash == Hash_POPVEC && Interpreter_stackPointers[currentStackIndex] <= NumDataFields-1) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    break;
                }
                PopTurtle_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                if (commandHash == Hash_PEEKVEC) {
                    Interpreter_stackPointers[currentStackIndex] += NumDataFields;
                }
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Main_stackPointers[currentStackIndex]);
                #endif
                break;                
            case Hash_IF:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No predicate.");
                    break;
                }
                if (!param1Val) {
                    skipFlag = true;
                }
                break;
            case Hash_ZERO:
                errNo = os_SetRealVar(param1Var, &Const_Real0);
                if (errNo) {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to zero out %c: %d.", param1Var[0], errNo);    
                }
                break;
            case Hash_INC:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2)
                        param2 = &Const_Real1;
                    *param1 = os_RealAdd(param1, param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);    
                }
                break;
            case Hash_DEC:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2)
                        param2 = &Const_Real1;
                    *param1 = os_RealSub(param1, param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);    
                }
                break;
            case Hash_STO:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    break;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    if (!param2) {
                        dbg_printf("\nSYNTAX ERROR: No value to set.");
                        break;
                    }
                    *param1 = os_RealCopy(param2);
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);    
                }
                break;
            case Hash_TURTLE:
                param1Int = (int24_t)param1Val;
                if (param1Int < -1 || param1Int > NumTurtles) {
                    dbg_printf("\nSYNTAX ERROR: Invalid turtle number %d.", param1Int);
                    break;
                }
                currentTurtleIndex = param1Int;
                break;
            case Hash_STACK:
                param1Int = (int24_t)param1Val;
                if (param1Int < -1 || param1Int > NumStackPages) {
                    dbg_printf("\nSYNTAX ERROR: Invalid stack number %d.", param1Int);
                    break;
                }
                currentStackIndex = param1Int;
                break;
            case Hash_FADEOUT:
                param1Int = (int24_t)param1Val;
                Palette_FadeOut(Interpreter_paletteBuffer, 0, 255, param1Int);
                break;
            case Hash_FADEIN:
                param1Int = (int24_t)param1Val;
                Pallete_FadeIn(Interpreter_paletteBuffer, 0, 255, param1Int);
                break;
            case Hash_PALETTE:
                param1Int = (int24_t)param1Val;
                switch (param1Int) {
                    case 0:
                        Palette_Default(Interpreter_paletteBuffer);
                        break;
                    default:
                        dbg_printf("\nSYNTAX ERROR: Invalid palette %d.", param1Int);
                        break;
                }
                gfx_SetPalette(Interpreter_paletteBuffer, 256, 0);
                break;
            case Hash_FILL:
                gfx_FloodFill(currentTurtle->X, currentTurtle->Y, currentTurtle->Color);
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
            case Hash_GETKEY:
                param1Int = keyhelper_GetKey();
                if (param1 == NULL) {
                    retList[0] = (float)param1Int;
                    retListPointer = 1;
                } else {
                    *param1 = os_Int24ToReal(param1Int);
                    errNo = os_SetRealVar(param1Var, param1);
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf(" Wrote %d to %c ", param1Int, param1Var[0]);
                    #endif
                    if (errNo) {
                        dbg_printf("\nSYNTAX ERROR: Got error trying to write %c: %d.", param1Var[0], errNo);
                    }
                }
                break;
            case Hash_KEYDOWN:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                param1Int = (int24_t)param1Val;
                retList[0] = keyhelper_IsDown(param1Int);
                retListPointer = 1;
                break;
            case Hash_IFKEYDOWN:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                param1Int = (int24_t)param1Val;
                if (!keyhelper_IsDown(param1Int)) {
                    skipFlag = true;
                }
                break;
            case Hash_KEYUP:
                param1Int = (int24_t)param1Val;
                retList[0] = keyhelper_IsUp(param1Int);
                retListPointer = 1;
                break;
            case Hash_IFKEYUP:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                param1Int = (int24_t)param1Val;
                if (!keyhelper_IsUp(param1Int)) {
                    skipFlag = true;
                }
                break;
            case Hash_KEYSCAN:
                kb_Scan();
                break;
            case Hash_SIZESPRITE:
                param1Int = (int24_t)param1Val;
                if (param1Int < 0 || param1Int > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", param1Int);
                    goto end_eval;
                }
                param2Int = (int24_t)param2Val;
                intEval = os_RealToInt24(&paramsList->items[2]);
                if (Interpreter_spriteDictionary[param1Int] != NULL)
                    free(Interpreter_spriteDictionary[param1Int]);
                Interpreter_spriteDictionary[param1Int] = gfx_MallocSprite(param2Int, intEval);
                break;
            case Hash_DEFSPRITE:
                param1Int = (int24_t)param1Val;
                if (param1Int < 0 || param1Int > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", param1Int);
                    goto end_eval;
                }
                param1Int = (int24_t)param1Val;
                if (param1Int < 0 || param1Int > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", param1Int);
                    goto end_eval;
                }
                break;
            default:
                dbg_printf("\nSYNTAX ERROR: Unknown hash encountered 0x%.6lX command %.*s.", commandHash, commandLength, command);
                break;
        }
        
        if (retListPointer == 0)
            goto end_eval;

        if (retListPointer == 1) {
            real_t ans = os_FloatToReal(retList[0]);
            #ifdef DEBUG_PROCESSOR
                dbg_printf("Ans: %.2f", retList[0]);
            #endif
            os_SetRealVar(OS_VAR_ANS, &ans);
            goto end_eval;
        }

        #ifdef DEBUG_PROCESSOR
            dbg_printf("Ans: ");
        #endif

        errNo = os_SetListDim(OS_VAR_ANS, retListPointer);
        if (errNo) {
            dbg_printf("\nSYNTAX ERROR: Got error trying to set ans dim %d.", retListPointer);
            goto end_eval;
        }

        for (uint16_t retListIndex = 0; retListIndex < retListPointer; retListIndex++)
        {
            #ifdef DEBUG_PROCESSOR
            dbg_printf("%.2f ", retList[retListIndex]);
            #endif
            real_t ans = os_FloatToReal(retList[retListIndex]);
            errNo = os_SetRealListElement(OS_VAR_ANS, retListIndex+1, &ans);
            if (errNo) {
                dbg_printf("\nSYNTAX ERROR: Got error trying to set ans index %d.", retListIndex);
                break;
            }
        }

end_eval:
        #ifdef DEBUG_PROCESSOR
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
            Turtle_Draw_InLine(&Interpreter_turtles[i]);
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

    
    gfx_End();
}