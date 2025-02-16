#include <graphx.h>
#include <string.h>
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

#include <debug.h>
#ifdef DEBUG
//#define DEBUG_PROCESSOR
#include <fileioc.h>

void debug_print_tokens(const void* buffer, size_t length, size_t* stringLength) {
    uint8_t tokenLength = 0;
    size_t tokenStringLength = 0;
    uint8_t i = 0;
    if (stringLength)
        *stringLength = 0;
        
    void** readPointer = (void**)&buffer;
    while(i < length) {
        dbg_printf("%s", ti_GetTokenString(readPointer, &tokenLength, &tokenStringLength));
        i += tokenLength;
        if (stringLength)
            *stringLength += tokenStringLength;
    }
}
#endif

void print_string(size_t length, const unsigned char buffer[length], const Turtle* turtle) {
    #ifdef DEBUG_PROCESSOR
    dbg_printf(" Ans text (%d): ", length);
    debug_print_tokens(buffer, length, NULL);
    dbg_printf(" ");
    #endif
    gfx_SetTextFGColor(turtle->Color);
    gfx_SetTextXY(turtle->X, turtle->Y);
    uint8_t tokenLength = 0;
    size_t tokenStringLength = 0;
    uint8_t i = 0;
    size_t* stringLength = 0;
        
    void** readPointer = (void**)&buffer;
    while(i < length) {
        gfx_PrintString(ti_GetTokenString(readPointer, &tokenLength, &tokenStringLength));
        i += tokenLength;
        if (stringLength)
            *stringLength += tokenStringLength;
    }
}

// should be treated as private
Turtle Interpreter_turtles[NumTurtles];
ProgramCounter Interpreter_labels[NumLabels];
StackPointer Interpreter_stackPointers[NumStackPages];
StackPointer Interpreter_systemStackPointer;
float Interpreter_stacks[NumStackPages][MaxStackDepth];
float Interpreter_systemStack[SystemStackDepth];
uint16_t Interpreter_paletteBuffer[256];
gfx_sprite_t* Interpreter_spriteDictionary[NumSprites];

__attribute__((hot))
void Interpreter_Interpret(size_t programSize, ProgramToken program[programSize]) {
    #ifdef DEBUG
    dbg_printf("Interpreter_Interpret: %p, %d\n", program, programSize);
    #endif
    
    ProgramCounter programCounter = 0;
    // Header
    if (program[0] == OS_TOK_COLON) {
        // We've detected the DCS header
        Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
        Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);    
    }
    Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
    if (programCounter > programSize) {
        return;
    }
    // Comment
    ProgramToken* comment = &program[programCounter];
    size_t commentLength = Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
    ProgramCounter programStart = programCounter;
    
    #ifdef DEBUG
    dbg_printf("\nsizeof(Turtle): %d\n", sizeof(Turtle));
    dbg_printf("\n");
    dbg_printf("pc:             0x%.6X\n", (uint24_t)&programCounter);
    dbg_printf("turtles:        0x%.6X\n", (uint24_t)Interpreter_turtles);
    dbg_printf("system stack:   0x%.6X\n", (uint24_t)Interpreter_systemStack);
    dbg_printf("system sp:      0x%.6X\n", (uint24_t)&Interpreter_systemStackPointer);
    dbg_printf("stacks:         0x%.6X\n", (uint24_t)Interpreter_stacks);
    dbg_printf("sp:             0x%.6X\n", (uint24_t)Interpreter_stackPointers);
    dbg_printf("labels:         0x%.6X\n", (uint24_t)Interpreter_labels);
    dbg_printf("palette:        0x%.6X\n", (uint24_t)Interpreter_paletteBuffer);
    dbg_printf("\n");
    debug_print_tokens(comment, commentLength, NULL);
    dbg_printf("\n");

    ProgramCounter dbgPc = programCounter;
    while (dbgPc < programSize) {
        dbg_printf("%.6X: ", dbgPc);
        ProgramCounter start = dbgPc;
        size_t lineLength = Seek_ToNewLine(program, programSize, Token_NewLine, &dbgPc);
        debug_print_tokens(&program[start], lineLength, NULL);
        dbg_printf("\n");
    }
    #endif

    gfx_Begin();
    srand(rtc_Time());
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[14] = "FPS: XXX     ";

    memset(Interpreter_spriteDictionary, 0, sizeof(gfx_sprite_t*)*NumSprites);
    
    Palette_Default(Interpreter_paletteBuffer); 
    #ifdef DEBUG
    Interpreter_paletteBuffer[1] = gfx_RGBTo1555(0, 125, 0);
    #endif  

program_start:
    gfx_SetPalette(Interpreter_paletteBuffer, 512, 0);
    
    gfx_SetDrawScreen();
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(1);

    memset(Interpreter_labels, 0, sizeof(ProgramCounter)*NumLabels);
    Interpreter_labels[255] = 0xABCDEF;
    memset(Interpreter_systemStack, 0, sizeof(float)*SystemStackDepth);
    memset(Interpreter_stackPointers, 0, sizeof(StackPointer)*NumStackPages);
    memset(Interpreter_stacks, 0, sizeof(float)*NumStackPages*MaxStackDepth);
    Interpreter_systemStackPointer = 0;
    memset(Interpreter_turtles, 0, sizeof(Turtle) * NumTurtles);
    Turtle_Initialize(Interpreter_turtles);
    
    programCounter = programStart;

    TurtleIndex currentTurtleIndex = 0;
    StackIndex currentStackIndex = 0;
        
    real_t* param1 = NULL;
    float eval;
    int24_t intEval;
    char param1Var[4];
    param1Var[0] = 0;
    param1Var[1] = 0;
    param1Var[2] = 0;
    param1Var[3] = 0;
    float paramsListFloats[0x4];
    int24_t paramsListInts[0x4];
    memset(paramsListFloats, 0, 0x4*sizeof(float));
    memset(paramsListInts, 0, 0x4*sizeof(int24_t));

    uint8_t type;
    void* ans;
    uint16_t paramsListLength;
    list_t* paramsList;
    cplx_list_t* paramsListCplx;

    float retList[MaxStackDepth];
    uint16_t retListPointer = 0;

    bool exit = false;
    bool running = true;
    bool showFps = true;
    bool skipFlag = false;

    #ifdef DEBUG
    dbg_printf("Starting program exection.\n");
    #endif
    Turtle* currentTurtle = &Interpreter_turtles[0];
    while (!exit) {
        if (!running)
            goto end_eval;
        
        #ifdef DEBUG_PROCESSOR
        dbg_printf("%.6X: ", programCounter);
        #endif

        ProgramToken* command = &program[programCounter];
        size_t commandStringLength = 0;
        uint24_t commandHash = 0;
        TugaOpCode opCode = toc_NOP;
        ProgramToken* params;
        size_t paramsStringLength = 0;

        ProgramToken shortHand = program[programCounter];
        switch (shortHand) {
            case Token_NewLine:
                #ifdef DEBUG_PROCESSOR
                dbg_printf("Skipping empty line.");
                #endif
                goto end_eval;
            case Token_Comment:
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("Skipping comment.");
                #endif
                goto end_eval;
            case Token_Label:
            case Token_Goto:
            case Token_LabelOs:
            case Token_GotoOs:
            case Token_If:
            case Token_IfOs:
            case Token_Ret:
            case Token_GoSub:
            case Token_StopOs:
            case Token_Push:
            case Token_Pop:
            case Token_Turtle:
            case Token_Sto:
            case Token_Left:
            case Token_LeftOs:
            case Token_Right:
            case Token_Forward:
                switch (shortHand) {
                    case Token_Forward:
                        opCode = toc_FORWARD;
                        break;
                    case Token_Right:
                        opCode = toc_RIGHT;
                        break;
                    case Token_Left:
                    case Token_LeftOs:
                        opCode = toc_LEFT;
                        break;
                    case Token_Sto:
                        opCode = toc_STO;
                        break;
                    case Token_Turtle:
                        opCode = toc_TURTLE;
                        break;
                    case Token_Push:
                        opCode = toc_PUSH;
                        break;
                    case Token_Pop:
                        opCode = toc_POP;
                        break;
                    case Token_StopOs:
                        opCode = toc_STOP;
                        break;
                    case Token_Ret:
                        opCode = toc_RET;
                        break;
                    case Token_GoSub:
                        opCode = toc_GOSUB;
                        break;
                    case Token_Label:
                    case Token_LabelOs:
                        opCode = toc_LABEL;
                        break;
                    case Token_Goto:
                    case Token_GotoOs:
                        opCode = toc_GOTO;
                        break;
                    case Token_If:
                    case Token_IfOs:
                        opCode = toc_IF;
                        break;
                }
                commandStringLength = 1;
                programCounter++;
                break;
            default:
                commandStringLength = Seek_ToNewLine(program, programSize, Token_Space, &programCounter);
                commandHash = Hash_InLine(command, commandStringLength);
                opCode = GetOpCodeFromHash_Inline(commandHash);
                break;
        }
        
        if (program[programCounter-1] != Token_NewLine) {
            params = &program[programCounter];
            paramsStringLength = Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
        }

        #ifdef DEBUG_PROCESSOR
        size_t outputTokenStringLength;
        debug_print_tokens(command, commandStringLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 10) {
            dbg_printf(" ");
            outputTokenStringLength++;
        }

        debug_print_tokens(params, paramsStringLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 30) {
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
        
        paramsListFloats[0] = 0;
        intEval = 0;
        param1Var[0] = params[0];
        param1 = NULL;
        paramsList = NULL;
        paramsListCplx = NULL;
        retListPointer = 0;
        ans = NULL;

        if (paramsStringLength > 0 && params[0] != OS_TOK_DOUBLE_QUOTE) {
            if (os_Eval(params, paramsStringLength)) {
                dbg_printf("\nSYNTAX ERROR: Failed to eval \"%.*s\" length: %d.", paramsStringLength, (const char*)params, paramsStringLength);
                goto end_eval;
            }

            ans = os_GetAnsData(&type);

            if (ans) {
                switch (type) {
                    case OS_TYPE_REAL:
                        paramsListLength = 1;
                        param1 = ans;
                        break;
                    case OS_TYPE_CPLX:
                        paramsListLength = 1;
                        param1 = ans;
                        break;
                    case OS_TYPE_REAL_LIST:
                        param1Var[0] = (char)*(params + 1);
                        paramsList = (list_t*)ans;
                        paramsListLength = paramsList->dim;
                        if (paramsList->dim >= 1)
                            param1 = &paramsList->items[0];
                        break;
                    case OS_TYPE_CPLX_LIST:
                        param1Var[0] = (char)*(params + 1);
                        paramsListCplx = (cplx_list_t*)ans;
                        paramsListLength = paramsListCplx->dim;
                        if (paramsListCplx->dim >= 1)
                            param1 = &paramsListCplx->items[0].real;
                        break;
                    default:
                        dbg_printf("\nSYNTAX ERROR: Unsupported ans type: %d.\n", type);
                        goto end_eval;
                }
            } else {
                dbg_printf("\nUNKNOWN ERROR: Failed to resolve ans.\n");
                goto end_eval;
            }
            
            if (param1 != NULL) {
                eval = paramsListFloats[0] = os_RealToFloat(param1);
                intEval = paramsListInts[0] = os_RealToInt24(param1);
                #ifdef DEBUG_PROCESSOR
                dbg_printf(" param1: %f ", eval);
                #endif
            }
        }
        
        #define getListElementPointerOrDefaultPointer(i,d) (paramsList == NULL ? paramsListCplx == NULL ? &d : &paramsListCplx->items[i].real : &paramsList->items[i])
        #define getListElementFloatOrDefault(i,d) (paramsList == NULL ? paramsListCplx == NULL ? d : os_RealToFloat(&paramsListCplx->items[i].real) : os_RealToFloat(&paramsList->items[i]))
        #define getListElementIntOrDefault(i,d) (paramsList == NULL ? paramsListCplx == NULL ? d : os_RealToInt24(&paramsListCplx->items[i].real) : os_RealToInt24(&paramsList->items[i]))

        gfx_SetColor(currentTurtle->Color);
        int errNo;
        switch (opCode) {
            case toc_NOP:
                break;
            case toc_COLOR:
                Turtle_SetColor(currentTurtle, &eval);
                break;
            case toc_PEN:
                Turtle_SetPen(currentTurtle, &eval);
                break;
            case toc_FORWARD:
                Turtle_Forward(currentTurtle, &eval);
                break;
            case toc_LEFT:
                Turtle_Left(currentTurtle, &eval);
                break;
            case toc_RIGHT:
                Turtle_Right(currentTurtle, &eval);
                break;
            case toc_MOVE:
                if (paramsListLength < 2) {
                    dbg_printf("\nSYNTAX ERROR: Missing parameters. Needed 2, found %d.", paramsListLength);
                    goto end_eval;
                }
                paramsListFloats[1] = getListElementFloatOrDefault(1, 0);
                Turtle_Goto(currentTurtle, &paramsListFloats[0], &paramsListFloats[1]);
                break;
            case toc_ANGLE:
                Turtle_SetAngle(currentTurtle, &eval);
                break;
            case toc_CIRCLE:
                intEval = if_null_then_a_else_b(param1, 1, intEval);
                if (currentTurtle->Pen)
                    gfx_FillCircle(currentTurtle->X, currentTurtle->Y, intEval);
                else
                    gfx_Circle(currentTurtle->X, currentTurtle->Y, intEval);
                break;
            case toc_RECT:
                paramsListInts[0] = getListElementIntOrDefault(0, 1);
                paramsListInts[1] = getListElementIntOrDefault(1, 1);
                if (currentTurtle->Pen)
                    gfx_FillRectangle(currentTurtle->X, currentTurtle->Y, paramsListInts[0], paramsListInts[1]);
                else
                    gfx_Rectangle(currentTurtle->X, currentTurtle->Y, paramsListInts[0], paramsListInts[1]);
                break;
            case toc_STOP:
                exit = true;
                break;
            case toc_CLEAR:
                intEval = if_null_then_a_else_b(param1, 0, intEval);
                gfx_FillScreen(paramsListInts[0] % 256);
                break;
            case toc_LABEL:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No label.");
                    goto end_eval;
                }
                if (intEval >= 0 && intEval < NumLabels)
                    Interpreter_labels[intEval] = programCounter;
                break;
            case toc_GOSUB:
            case toc_GOTO:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No label.");
                    goto end_eval;
                }
                if (intEval >= NumLabels) {
                    dbg_printf("\nSYNTAX ERROR: Invalid label: %d.", intEval);
                    goto end_eval;
                }
                size_t labelIndex = Interpreter_labels[intEval];
                if (labelIndex <= programStart || labelIndex >= programSize) {
                    labelIndex = Seek_ToLabel(program, programSize, programStart, intEval);
                    if (labelIndex <= programStart || labelIndex >= programSize)
                    {
                        dbg_printf("\nSYNTAX ERROR: Label not found: %d - %d.", intEval, labelIndex);
                        goto end_eval;
                    }
                    Interpreter_labels[intEval] = labelIndex;
                }

                if (opCode == toc_GOSUB) {
                    float pc = (float)programCounter;
                    Push_InLine(Interpreter_systemStack, &Interpreter_systemStackPointer, &pc);
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("pushing PC onto stack %f", pc);
                    #endif
                }
                programCounter = labelIndex;
                break;
            case toc_EVAL:
                dbg_printf(" *");
                break;
            case toc_PUSH:
                if (paramsList == NULL) {
                    if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                        dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                        goto end_eval;
                    }
                    Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &paramsListFloats[0]);
                } else {
                    for (uint16_t pushListIndex = 0; pushListIndex < paramsList->dim; pushListIndex++)  {
                        if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                            dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                            goto end_eval;
                        }
                        paramsListFloats[0] = os_RealToFloat(&paramsList->items[pushListIndex]);
                        Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &paramsListFloats[0]);
                    }
                }
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Interpreter_stackPointers[currentStackIndex]);
                #endif
                break;
            case toc_RET:
                if (Interpreter_systemStackPointer == 0) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for system stack.");
                    goto end_eval;
                }
                eval = Pop_InLine(Interpreter_systemStack, &Interpreter_systemStackPointer);
                programCounter = (size_t)eval;
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("setting PC from stack %f pc %d", eval, programCounter);
                #endif
                break;
            case toc_POP:
            case toc_PEEK:
                if (opCode == toc_POP && Interpreter_stackPointers[currentStackIndex] == 0) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    goto end_eval;
                }
                if (opCode == toc_PEEK) {
                    eval = Peek_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex]);
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf("peeked: %f", eval);
                    dbg_printf(" sp: %d", Interpreter_stackPointers[currentStackIndex]);
                    #endif
                } else {
                    eval = Pop_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex]);
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("popped: %f", eval);
                        dbg_printf(" sp: %d", Interpreter_stackPointers[currentStackIndex]);
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
                        goto end_eval; 
                    }
                }
                break;
            case toc_PUSHVEC:
                if (Interpreter_stackPointers[currentStackIndex] + 6 >= MaxStackDepth) {
                    dbg_printf("\nSYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                    goto end_eval;
                }
                PushTurtle_Inline(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Interpreter_stackPointers[currentStackIndex]);
                #endif
                break;
            case toc_POPVEC:
            case toc_PEEKVEC:
                if (opCode == toc_POPVEC && Interpreter_stackPointers[currentStackIndex] <= NumDataFields-1) {
                    dbg_printf("\nSYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    goto end_eval;
                }
                if (opCode == toc_PEEKVEC) {
                    PeekTurtle_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                } else {
                    PopTurtle_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                }
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Interpreter_stackPointers[currentStackIndex]);
                #endif
                break;                
            case toc_IF:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No predicate.");
                    goto end_eval;
                }
                if (!paramsListFloats[0]) {
                    skipFlag = true;
                }
                break;
            case toc_ZERO:
                errNo = os_SetRealVar(param1Var, &Const_Real0);
                if (errNo) {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to zero out %c: %d.", param1Var[0], errNo);   
                    goto end_eval; 
                }
                break;
            case toc_INC:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    goto end_eval;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    *param1 = os_RealAdd(param1, getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);
                    goto end_eval;  
                }
                break;
            case toc_DEC:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    goto end_eval;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    *param1 = os_RealSub(param1, getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);
                    goto end_eval;
                }
                break;
            case toc_STO:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No parameter to set.");
                    goto end_eval;
                }
                errNo = os_GetRealVar(param1Var, param1);
                if (!errNo) {
                    *param1 = os_RealCopy(getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(param1Var, param1);
                } else {
                    dbg_printf("\nSYNTAX ERROR: Got error trying to read %c: %d.", param1Var[0], errNo);
                    goto end_eval;
                }
                break;
            case toc_TURTLE:
                if (intEval < -1 || intEval > NumTurtles) {
                    dbg_printf("\nSYNTAX ERROR: Invalid turtle number %d.", intEval);
                    goto end_eval;
                }
                currentTurtleIndex = intEval;
                currentTurtle = &Interpreter_turtles[currentTurtleIndex];
                if (!currentTurtle->Initialized)
                    Turtle_Initialize(currentTurtle);
                break;
            case toc_STACK:
                if (intEval < -1 || intEval > NumStackPages) {
                    dbg_printf("\nSYNTAX ERROR: Invalid stack number %d.", intEval);
                    goto end_eval;
                }
                currentStackIndex = intEval;
                break;
            case toc_FADEOUT:
                Palette_FadeOut(Interpreter_paletteBuffer, 0, 255, intEval);
                break;
            case toc_FADEIN:
                Pallete_FadeIn(Interpreter_paletteBuffer, 0, 255, intEval);
                break;
            case toc_PALETTE:
                switch (intEval) {
                    case 0:
                        Palette_Default(Interpreter_paletteBuffer);
                        break;
                    case 1:
                        Palette_Rainbow(Interpreter_paletteBuffer);
                        break;
                    default:
                        dbg_printf("\nSYNTAX ERROR: Invalid palette %d.", intEval);
                        goto end_eval;
                }
                gfx_SetPalette(Interpreter_paletteBuffer, 512, 0);
                break;
            case toc_PALSHIFT:
                Palette_Shift(Interpreter_paletteBuffer);
                gfx_SetPalette(Interpreter_paletteBuffer, 512, 0);
                break;
            case toc_FILL:
                gfx_FloodFill(currentTurtle->X, currentTurtle->Y, currentTurtle->Color);
                break;
            case toc_BLITSCREEN:
                gfx_BlitScreen();
                break;
            case toc_BLITBUFFER:
                gfx_BlitBuffer();
                break;
            case toc_DRAWBUFFER:
                gfx_SetDrawBuffer();
                break;
            case toc_DRAWSCREEN:
                gfx_SetDrawScreen();
                break;
            case toc_SWAPDRAW:
                gfx_SwapDraw();
                break;
            case toc_INIT:
                Turtle_Initialize(currentTurtle);
                break;
            case toc_GETKEY:
                intEval = keyhelper_GetKey();
                if (param1 == NULL) {
                    retList[0] = (float)intEval;
                    retListPointer = 1;
                } else {
                    *param1 = os_Int24ToReal(intEval);
                    errNo = os_SetRealVar(param1Var, param1);
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf(" Wrote %d to %c ", intEval, param1Var[0]);
                    #endif
                    if (errNo) {
                        dbg_printf("\nSYNTAX ERROR: Got error trying to write %c: %d.", param1Var[0], errNo);
                    }
                }
                break;
            case toc_KEYDOWN:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                retList[0] = keyhelper_IsDown(intEval);
                retListPointer = 1;
                break;
            case toc_IFKEYDOWN:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                if (!keyhelper_IsDown(intEval)) {
                    skipFlag = true;
                }
                break;
            case toc_KEYUP:
                retList[0] = keyhelper_IsUp(intEval);
                retListPointer = 1;
                break;
            case toc_IFKEYUP:
                if (param1 == NULL) {
                    dbg_printf("\nSYNTAX ERROR: No key value.");
                    goto end_eval;
                }
                if (!keyhelper_IsUp(intEval)) {
                    skipFlag = true;
                }
                break;
            case toc_KEYSCAN:
                kb_Scan();
                break;
            case toc_SIZESPRITE:
                if (intEval < 0 || intEval > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", intEval);
                    goto end_eval;
                }
                break;
            case toc_DEFSPRITE:
                if (intEval < 0 || intEval > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", intEval);
                    goto end_eval;
                }
                if (intEval < 0 || intEval > NumSprites)
                {
                    dbg_printf("\nSYNTEAX ERROR: Out of range of sprites %d.", intEval);
                    goto end_eval;
                }
                break;
            case toc_TEXT:
                print_string(paramsStringLength - 1, &params[1], currentTurtle);
                break;
            case toc_UNKNOWN:
                dbg_printf("\nSYNTAX ERROR: Unknown hash encountered 0x%.6lX command %.*s.", (uint32_t)commandHash, commandStringLength, command);
                break;
            default:
                dbg_printf("\nUNIMPLEMENETED OPCODE %d %.*s.", opCode, commandStringLength, command);
                break;
        }
        
        if (retListPointer == 0)
            goto end_eval;

        real_t ansEntry;
        if (retListPointer == 1) {
            ansEntry = os_FloatToReal(retList[0]);
            #ifdef DEBUG_PROCESSOR
                dbg_printf("Ans: %.2f", retList[0]);
            #endif
            os_SetRealVar(OS_VAR_ANS, &ansEntry);
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
            ansEntry = os_FloatToReal(retList[retListIndex]);
            errNo = os_SetRealListElement(OS_VAR_ANS, retListIndex+1, &ansEntry);
            if (errNo) {
                dbg_printf("\nSYNTAX ERROR: Got error trying to set ans index %d.", retListIndex);
                goto end_eval;
            }
        }

end_eval:
        #ifdef DEBUG_PROCESSOR
        if (running)
            dbg_printf("\n");
        #endif
        if (programCounter >= programSize) {
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
            gfx_SetTextFGColor(124);
            gfx_PrintStringXY("Paused ", 4, 4);
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
            gfx_SetTextFGColor(124);
            snprintf(buffer, 14, "FPS: %d   ", (uint8_t)fps);
            gfx_PrintStringXY(buffer, 4, 4);
        }
        
        //gfx_SwapDraw();
    }

    dbg_printf("Done. PC: %.6X\n", programCounter);
    gfx_BlitScreen();
    gfx_SetTextFGColor(124);
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