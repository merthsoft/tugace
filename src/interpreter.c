#include <fileioc.h>
#include <graphx.h>
#include <string.h>
#include <time.h>

#include <sys/rtc.h>

#include <ti/error.h>
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

void debug_print_tokens(const void* buffer, size_t bufferLength, size_t* stringLength) {
    size_t runningStringLength = 0;
    if (stringLength == NULL)
        stringLength = &runningStringLength;
    uint8_t tokenLength = 0;
    size_t returnedStringLength = 0;
    size_t i = 0;
    if (stringLength)
        *stringLength = 0;
        
    void** readPointer = (void**)&buffer;
    char printBuffer[51];
    while(i < bufferLength && *stringLength < 50) {
        char* retStr = ti_GetTokenString(readPointer, &tokenLength, &returnedStringLength);
        strncpy(&printBuffer[*stringLength], retStr, returnedStringLength);
        i += tokenLength;
        *stringLength += returnedStringLength;
    }
    printBuffer[*stringLength] = 0;
    dbg_printf("%.*s", *stringLength, printBuffer);
    if (i < bufferLength)
        dbg_printf("...");
}
#endif

__attribute__((hot))
static inline void Interpreter_printString(size_t length, const uint8_t buffer[length], const Turtle* turtle) {
    gfx_SetTextFGColor(turtle->Color);
    gfx_SetTextXY(turtle->X, turtle->Y);
    uint8_t tokenLength = 0;
    size_t tokenStringLength = 0;
    size_t i = 0;
        
    void** readPointer = (void**)&buffer;
    while(i < length) {
        gfx_PrintString(ti_GetTokenString(readPointer, &tokenLength, &tokenStringLength));
        i += tokenLength;
    }
}

__attribute__((hot))
void Interpreter_copySprite(size_t dataLength, const unsigned char data[dataLength], gfx_sprite_t* sprite) {
    for (size_t i = 0; i < dataLength; i+=2) {
        if (i/2 >= sprite->height * sprite->width) {
            dbg_printf("ERROR: Sprite data exceeded size.");
        }
        
        sprite->data[i/2] = convertHexPairToByte(data, i);
    }
}

#define Interpreter_asmProgramBufferSize 512
uint8_t Interpreter_asmProgramBuffer[Interpreter_asmProgramBufferSize];
typedef void (*TurtleFunction(const Turtle*));

__attribute__((hot))
static inline void Interpreter_execAsmString(size_t dataLength, const uint8_t data[dataLength], const Turtle* turtle) {
    return;
    // TODO: Fill in function header and offset
    size_t i;
    for (i = 0; i < dataLength && i < Interpreter_asmProgramBufferSize*2; i+=2) {
        Interpreter_asmProgramBuffer[i/2] = convertHexPairToByte(data, i);
    }
    if (i >= Interpreter_asmProgramBufferSize*2) {
        return;
    }
    // TODO: Fill in function footer
    TurtleFunction* asmProgram = (TurtleFunction*)Interpreter_asmProgramBuffer;
    asmProgram(turtle);
}

typedef struct NamedLabel {
    ProgramCounter ProgramCounter;
    uint24_t Hash;
} NamedLabel;

// should be treated as private
static Turtle Interpreter_turtles[NumTurtles];
static NamedLabel Interpreter_labels[NumLabels];
static StackPointer Interpreter_stackPointers[NumStackPages];
static StackPointer Interpreter_systemStackPointer;
static float Interpreter_stacks[NumStackPages][MaxStackDepth];
static float Interpreter_systemStack[SystemStackDepth];
static gfx_sprite_t* Interpreter_spriteDictionary[NumSprites];

static char errorMessage[256];
#define errorMessageLength 256

__attribute__((hot))
void Interpreter_Interpret(size_t programBufferSize, ProgramToken program[programBufferSize], size_t programSize) {
    #ifdef DEBUG
    dbg_printf("Interpreter_Interpret: program[%d]: %p.\n", programSize, program);
    #endif

    if (programSize == 0)
        return;
    
    ProgramCounter programCounter = 0;
    
    if (program[programCounter]       == OS_TOK_0
        && program[programCounter+1]  == OS_TOK_T
        && program[programCounter+2]  == OS_TOK_U
        && program[programCounter+3]  == OS_TOK_G
        && program[programCounter+4]  == OS_TOK_A) {

        Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
    }

    ProgramToken* comment = NULL;
    size_t commentLength = 0;
    // Header comments can be skipped entirely
    while (program[programCounter] == Token_Comment) {
        if (comment == NULL)
            comment = &program[programCounter];
        size_t lineLength = Seek_ToNewLine(program, programSize, Token_NewLine, &programCounter);
        if (lineLength > 100) {
            comment = NULL;
        } else if (commentLength == 0)
            commentLength = lineLength;
    }

    if (programCounter >= programSize) {
        return;
    }

    ProgramCounter programStart = programCounter;
    
    #ifdef DEBUG
    dbg_printf("\nsizeof(Turtle): %d\n", sizeof(Turtle));
    dbg_printf("sizeof(NamedLabel): %d\n", sizeof(NamedLabel));
    dbg_printf("\n");
    dbg_printf("palette:        0x%.6X\n", (uint24_t)Palette_PaletteBuffer);
    dbg_printf("pc:             0x%.6X\n", (uint24_t)&programCounter);
    dbg_printf("turtles:        0x%.6X\n", (uint24_t)Interpreter_turtles);
    dbg_printf("system stack:   0x%.6X\n", (uint24_t)Interpreter_systemStack);
    dbg_printf("system sp:      0x%.6X\n", (uint24_t)&Interpreter_systemStackPointer);
    dbg_printf("stacks:         0x%.6X\n", (uint24_t)Interpreter_stacks);
    dbg_printf("sp:             0x%.6X\n", (uint24_t)Interpreter_stackPointers);
    dbg_printf("labels:         0x%.6X\n", (uint24_t)Interpreter_labels);
    dbg_printf("last label:     0x%.6X\n", (uint24_t)&Interpreter_labels[NumLabels-1]);
    dbg_printf("sprite dict:    0x%.6X\n", (uint24_t)Interpreter_spriteDictionary);
    dbg_printf("\n");
    if (commentLength != 0) {
        debug_print_tokens(comment, commentLength, NULL);
        dbg_printf("\n");
    }
    
    #ifdef DEBUG_PROCESSOR
    ProgramCounter dbgPc = programCounter;
    while (dbgPc < programSize) {
        dbg_printf("%.8d: ", dbgPc);
        ProgramCounter start = dbgPc;
        size_t lineLength = Seek_ToNewLine(program, programSize, Token_NewLine, &dbgPc);
        debug_print_tokens(&program[start], lineLength, NULL);
        dbg_printf("\n");
    }
    #endif
    #endif

    srand(rtc_Time());
    clock_t time = clock();
    uint24_t framecount = 0;
    float fps = 0.0f;
    char buffer[4] = "XXX";
    
program_start:
    memset(Interpreter_spriteDictionary, 0, sizeof(gfx_sprite_t*)*NumSprites);
    
    Palette_Default(Palette_PaletteBuffer);
    gfx_SetPalette(Palette_PaletteBuffer, 512, 0);
    gfx_SetTextConfig(gfx_text_clip);
    
    gfx_SetDrawScreen();
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(1);
    gfx_FillScreen(0);

    memset(Interpreter_labels, 0, sizeof(NamedLabel)*NumLabels);
    memset(Interpreter_systemStack, 0, sizeof(float)*SystemStackDepth);
    memset(Interpreter_stackPointers, 0, sizeof(StackPointer)*NumStackPages);
    memset(Interpreter_stacks, 0, sizeof(float)*NumStackPages*MaxStackDepth);
    Interpreter_systemStackPointer = 0;
    memset(Interpreter_turtles, 0, sizeof(Turtle) * NumTurtles);
    Turtle_Initialize(Interpreter_turtles);
    
    programCounter = programStart;

    TurtleIndex currentTurtleIndex = 0;
    StackIndex currentStackIndex = 0;
        
    real_t* paramReal = NULL;
    float eval;
    int24_t intEval;
    char paramVar[4];
    paramVar[0] = 0;
    paramVar[1] = 0;
    paramVar[2] = 0;
    paramVar[3] = 0;

    bool exit = false;
    bool running = true;
    bool showFps = true;
    bool pauseOnError = true;
    bool autoDraw = true;
    bool skipFlag = false;
    
    SpriteIndex currentSpriteIndex = 0;
    Turtle* currentTurtle = &Interpreter_turtles[0];
    uint8_t type;
    void* ans;
    string_t* ansString;
    uint16_t paramsListLength;
    list_t* paramsList;
    cplx_list_t* paramsListCplx;
    ProgramToken* params;
    float retList[MaxStackDepth];

    #define getListElementPointerOrDefaultPointer(i,d)  (paramsList == NULL ? paramsListCplx == NULL ? &d : &paramsListCplx->items[i].real : &paramsList->items[i])
    #define getListElementFloatOrDefault(i,d)           (paramsList == NULL ? paramsListCplx == NULL ? d : os_RealToFloat(&paramsListCplx->items[i].real) : os_RealToFloat(&paramsList->items[i]))
    #define getListElementIntOrDefault(i,d)             (paramsList == NULL ? paramsListCplx == NULL ? d : os_RealToInt24(&paramsListCplx->items[i].real) : os_RealToInt24(&paramsList->items[i]))

    ProgramCounter lineStartPc;
    ProgramToken* command;
    uint16_t retListPointer;
    size_t commandStringLength;
    uint24_t commandHash;
    TugaOpCode opCode;
    size_t paramsStringLength;
    ProgramToken shortHand;
    uint24_t errNo;
    
    #ifdef DEBUG
    dbg_printf("Starting program exection.\n");
    #endif
    while (!exit) {
        if (!running)
            goto end_eval;
        
        #ifdef DEBUG_PROCESSOR
        dbg_printf("%.8d: ", programCounter);
        #endif
        
        retListPointer = 0;
        commandStringLength = 0;
        commandHash = 0;
        opCode = toc_NOP;
        paramsStringLength = 0;
        
        while (program[programCounter] == Token_Indent) {
            programCounter++;
        }
        
        lineStartPc = programCounter;
        command = &program[programCounter];
        shortHand = program[programCounter];
        
        switch (shortHand) {
            case Shorthand_Label:
            case Shorthand_Goto:
            case Shorthand_LabelOs:
            case Shorthand_GotoOs:
            case Shorthand_If:
            case Shorthand_IfOs:
            case Shorthand_Ret:
            case Shorthand_GoSub:
            case Shorthand_StopOs:
            case Shorthand_Push:
            case Shorthand_Pop:
            case Shorthand_Turtle:
            case Shorthand_Sto:
            case Shorthand_Left:
            case Shorthand_LeftOs:
            case Shorthand_Right:
            case Shorthand_Forward:
            case Token_NewLine:
            case Token_Comment:
                switch (shortHand) {
                    case Token_NewLine:
                        #ifdef DEBUG_PROCESSOR
                        dbg_printf("Empty line");
                        #endif
                        opCode = toc_NOP;
                        break;
                    case Token_Comment:
                        #ifdef DEBUG_PROCESSOR
                            dbg_printf("Comment");
                        #endif
                        opCode = toc_NOP;
                        break;
                    case Shorthand_Forward:
                        opCode = toc_FORWARD;
                        break;
                    case Shorthand_Right:
                        opCode = toc_RIGHT;
                        break;
                    case Shorthand_Left:
                    case Shorthand_LeftOs:
                        opCode = toc_LEFT;
                        break;
                    case Shorthand_Sto:
                        opCode = toc_STO;
                        break;
                    case Shorthand_Turtle:
                        opCode = toc_TURTLE;
                        break;
                    case Shorthand_Push:
                        opCode = toc_PUSH;
                        break;
                    case Shorthand_Pop:
                        opCode = toc_POP;
                        break;
                    case Shorthand_StopOs:
                        opCode = toc_STOP;
                        break;
                    case Shorthand_Ret:
                        opCode = toc_RET;
                        break;
                    case Shorthand_GoSub:
                        opCode = toc_GOSUB;
                        break;
                    case Shorthand_Label:
                    case Shorthand_LabelOs:
                        opCode = toc_LABEL;
                        break;
                    case Shorthand_Goto:
                    case Shorthand_GotoOs:
                        opCode = toc_GOTO;
                        break;
                    case Shorthand_If:
                    case Shorthand_IfOs:
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
        ProgramCounter lineEndPc = programCounter;

        #ifdef DEBUG_PROCESSOR
        size_t outputTokenStringLength;
        debug_print_tokens(command, commandStringLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 12) {
            dbg_printf(" ");
            outputTokenStringLength++;
        }

        debug_print_tokens(params, paramsStringLength, &outputTokenStringLength);
        
        while (outputTokenStringLength < 30) {
            dbg_printf(" ");
            outputTokenStringLength++;
        }
        #endif

        if (opCode == toc_NOP) {
            #ifdef DEBUG_PROCESSOR
                dbg_printf("Skipping because nopped.");
            #endif
            goto end_eval;
        }

        if (skipFlag) {
            #ifdef DEBUG_PROCESSOR
                dbg_printf("Skipping because skipFlag is set.");
            #endif
            skipFlag = false;
            goto end_eval;
        }
        
        eval = 0.0f;
        intEval = 0;
        
        paramVar[0] = params[0];
        // TODO: Get multi-token variables
        paramReal = NULL;
        paramsList = NULL;
        paramsListCplx = NULL;
        retListPointer = 0;
        ans = NULL;
        ansString = NULL;

        if (paramsStringLength == 0) {
            #ifdef DEUBG_PROCESSOR
            dbg_printf("no params", opCode);
            #endif
            goto skip_eval;
        }

        if (params[0] == Token_Flag_NoEvalParams) {
            #ifdef DEUBG_PROCESSOR
            dbg_printf("skipping eval because line starts with NoEvalParams", opCode);
            #endif
            goto skip_eval;
        }

        if (params[0] == Token_Flag_EvalParams) {
            params++;
            paramsStringLength--;
        } else if (toc_SkipEval(opCode)) {
            #ifdef DEUBG_PROCESSOR
            dbg_printf("skipping eval because %d is skippable opCode", opCode);
            #endif
            goto skip_eval;
        }

        if ((errNo = os_Eval(params, paramsStringLength))) {
            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Failed to eval \"%.*s\" length: %d error: %d.", paramsStringLength, (const char*)params, paramsStringLength, errNo & OS_E_MASK);
            goto syntax_error;
        }

        if (opCode == toc_EVAL) {
            #ifdef DEBUG_PROCESSOR
            dbg_printf("Eval'd.");
            #endif
            goto end_eval;
        }

        ans = os_GetAnsData(&type);

        if (ans) {
            switch (type) {
                case OS_TYPE_REAL:
                    paramsListLength = 1;
                    paramReal = ans;
                    break;
                case OS_TYPE_CPLX:
                    paramsListLength = 1;
                    paramReal = ans;
                    break;
                case OS_TYPE_REAL_LIST:
                    paramVar[0] = (char)*(params + 1);
                    paramsList = (list_t*)ans;
                    paramsListLength = paramsList->dim;
                    if (paramsList->dim >= 1)
                        paramReal = &paramsList->items[0];
                    break;
                case OS_TYPE_CPLX_LIST:
                    paramVar[0] = (char)*(params + 1);
                    paramsListCplx = (cplx_list_t*)ans;
                    paramsListLength = paramsListCplx->dim;
                    if (paramsListCplx->dim >= 1)
                        paramReal = &paramsListCplx->items[0].real;
                    break;
                case OS_TYPE_STR:
                    ansString = ans;
                    break;
                default:
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Unsupported ans type: %d.\n", type);
                    goto syntax_error;
            }
        } else {
            snprintf(errorMessage, errorMessageLength, "UNKNOWN ERROR: Failed to resolve ans.");
            goto syntax_error;
        }
        
        if (paramReal != NULL) {
            eval = os_RealToFloat(paramReal);
            intEval = os_RealToInt24(paramReal);
            #ifdef DEBUG_PROCESSOR
            if (paramsListLength == 1) {
                dbg_printf(" param: %f ", eval);
            } else {
                dbg_printf(" params: ");
                for (retListPointer = 0; retListPointer < paramsListLength; retListPointer++) {
                    dbg_printf("%.2f ", getListElementFloatOrDefault(retListPointer, 0));
                }
                retListPointer = 0;
            }
            #endif
        }
        #ifdef DEBUG_PROCESSOR
        else if (ansString != NULL) {
            dbg_printf(" param: ");
            debug_print_tokens(ansString->data, ansString->len, NULL);
            dbg_printf(" ");
        }
        #endif

skip_eval:
        if (currentTurtle == NULL) {
            snprintf(errorMessage, errorMessageLength, "UNEXPECTED ERROR: Current turtle pointer is somehow null.");
            goto syntax_error;
        }
        
        #ifdef DEBUG_PROCESSOR
        dbg_printf("* ");
        #endif
        
        switch (opCode) {
            case toc_NOP:
            case toc_EVAL:
                break;
            case toc_COLOR:
                Turtle_SetColor(currentTurtle, &eval);
                break;
            case toc_PEN:
                Turtle_SetPen(currentTurtle, &eval);
                break;
            case toc_WRAP:
                Turtle_SetWrap(currentTurtle, &eval);
                break;
            case toc_FORWARD:
                gfx_SetColor(currentTurtle->Color);
                if (paramsListLength == 1) {
                    Turtle_Forward(currentTurtle, &eval, autoDraw);
                } else if (paramsListLength == 2) {
                    eval += currentTurtle->X;
                    float y = currentTurtle->Y + getListElementFloatOrDefault(1, 0);
                    Turtle_Goto(currentTurtle, &eval, &y, autoDraw);
                }
                break;
            case toc_LEFT:
                Turtle_Left(currentTurtle, &eval);
                break;
            case toc_RIGHT:
                Turtle_Right(currentTurtle, &eval);
                break;
            case toc_MOVE:
                gfx_SetColor(currentTurtle->Color);
                if (paramsListLength < 2) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Missing parameters. Needed 2, found %d.", paramsListLength);
                    goto syntax_error;
                }
                float y = getListElementFloatOrDefault(1, 0);
                Turtle_Goto(currentTurtle, &eval, &y, autoDraw);
                break;
            case toc_ANGLE:
                Turtle_SetAngle(currentTurtle, &eval);
                break;
            case toc_CIRCLE:
                gfx_SetColor(currentTurtle->Color);
                intEval = if_null_then_a_else_b(paramReal, 1, intEval);
                if (currentTurtle->Pen)
                    gfx_FillCircle(currentTurtle->X, currentTurtle->Y, intEval);
                else
                    gfx_Circle(currentTurtle->X, currentTurtle->Y, intEval);
                break;
            case toc_ELLIPSE:{
                gfx_SetColor(currentTurtle->Color);
                uint24_t w = getListElementIntOrDefault(0, 1);
                uint24_t h = getListElementIntOrDefault(1, 1);
                if (currentTurtle->Pen)
                    gfx_FillEllipse(currentTurtle->X, currentTurtle->Y, w, h);
                else
                    gfx_Ellipse(currentTurtle->X, currentTurtle->Y, w, h);
                break;}
            case toc_RECT: {
                gfx_SetColor(currentTurtle->Color);
                uint24_t w = getListElementIntOrDefault(0, 1);
                uint24_t h = getListElementIntOrDefault(1, 1);
                if (currentTurtle->Pen)
                    gfx_FillRectangle(currentTurtle->X, currentTurtle->Y, w, h);
                else
                    gfx_Rectangle(currentTurtle->X, currentTurtle->Y, w, h);
                break; }
            case toc_STOP:
                exit = true;
                break;
            case toc_CLEAR:
                intEval = if_null_then_a_else_b(paramReal, 0, intEval);
                gfx_FillScreen(intEval % 256);
                break;
            case toc_LABEL:
                if (paramReal != NULL) {
                    if (intEval >= 0 && intEval < NumLabels) {
                        #ifdef DEBUG_PROCESSOR
                        dbg_printf("Setting label %d which is at %p to %d.", intEval, &Interpreter_labels[intEval], programCounter);
                        #endif
                        Interpreter_labels[intEval].ProgramCounter = programCounter;
                    }
                } else {
                    commandHash = Hash_InLine(params, paramsStringLength);
                    for (intEval = NumLabels - 1; intEval <= 0; intEval--) {
                        if (Interpreter_labels[intEval].Hash == commandHash) {
                            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Label hash conflict %X: %.*s.", commandHash, paramsStringLength, params);
                            goto syntax_error;
                        }
                        if (Interpreter_labels[intEval].ProgramCounter == 0) {
                            break;
                        }
                    }
                    if (intEval < 0 || intEval >= NumLabels) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Ran out of labels %d.", intEval);
                        goto syntax_error;
                    }
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf("Setting label %d which is at %p to %d.", intEval, &Interpreter_labels[intEval], programCounter);
                    #endif
                    Interpreter_labels[intEval].Hash = commandHash;
                    Interpreter_labels[intEval].ProgramCounter = programCounter;
                }
                break;
            case toc_GOSUB:
            case toc_GOTO:
                if (paramReal != NULL) {
                    if (intEval >= NumLabels || intEval < 0) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Invalid label: %d.", intEval);
                        goto syntax_error;
                    }
                    ProgramCounter newPc = Interpreter_labels[intEval].ProgramCounter;
                    if (newPc <= programStart || newPc >= programSize) {
                        newPc = Seek_ToLabel(programSize, program, programStart, 0, intEval);
                        if (newPc <= programStart || newPc >= programSize) {
                            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Label not found: %d - %d.", intEval, newPc);
                            goto syntax_error;
                        }
                        Interpreter_labels[intEval].ProgramCounter = newPc;
                    }
                    programCounter = newPc;
                } else {
                    commandHash = Hash_InLine(params, paramsStringLength);
                    LabelIndex lastEmpty = NumLabels;
                    for (intEval = NumLabels - 1; intEval >= 0; intEval--) {
                        if (Interpreter_labels[intEval].Hash == commandHash) {
                            programCounter = Interpreter_labels[intEval].ProgramCounter;
                            break;
                        }
                        if (Interpreter_labels[intEval].Hash == 0) {
                            lastEmpty = intEval;
                            break;
                        }
                    }

                    if (intEval == NumLabels) {
                        ProgramCounter newPc = Seek_ToLabel(programSize, program, programStart, commandHash, 0);
                        if (newPc <= programStart || newPc >= programSize) {
                            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Label not found: %d - %d.", newPc, newPc);
                            goto syntax_error;
                        }
                        Interpreter_labels[lastEmpty].ProgramCounter = newPc;
                        Interpreter_labels[lastEmpty].Hash = commandHash;
                        programCounter = newPc;
                    }
                }

                if (opCode == toc_GOSUB) {
                    float pc = (float)lineEndPc;
                    Push_InLine(Interpreter_systemStack, &Interpreter_systemStackPointer, &pc);
                    #ifdef DEBUG_PROCESSOR
                        dbg_printf("pushing PC onto stack %f", pc);
                    #endif
                }
                break;
            case toc_PUSH:
                if (paramsList == NULL) {
                    if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                        goto syntax_error;
                    }
                    Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &eval);
                } else {
                    for (uint16_t pushListIndex = 0; pushListIndex < paramsList->dim; pushListIndex++)  {
                        if (Interpreter_stackPointers[currentStackIndex] == MaxStackDepth-1) {
                            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                            goto syntax_error;
                        }
                        eval = os_RealToFloat(&paramsList->items[pushListIndex]);
                        Push_InLine(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], &eval);
                    }
                }
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Interpreter_stackPointers[currentStackIndex]);
                #endif
                break;
            case toc_RET:
                if (Interpreter_systemStackPointer == 0) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Negative stack depth for system stack.");
                    goto syntax_error;
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
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    goto syntax_error;
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
                if (paramReal == NULL) {
                    retList[0] = eval;
                    retListPointer++;
                } else {
                    *paramReal = os_FloatToReal(eval);
                    os_SetRealVar(paramVar, paramReal);
                }
                break;
            case toc_PUSHVEC:
                if (Interpreter_stackPointers[currentStackIndex] + 6 >= MaxStackDepth) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Max stack depth violated for stack number %d: %d.", currentStackIndex, Interpreter_stackPointers[currentStackIndex]);
                    goto syntax_error;
                }
                PushTurtle_Inline(Interpreter_stacks[currentStackIndex], &Interpreter_stackPointers[currentStackIndex], currentTurtle);
                #ifdef DEBUG_PROCESSOR
                    dbg_printf("new sp: %d", Interpreter_stackPointers[currentStackIndex]);
                #endif
                break;
            case toc_POPVEC:
            case toc_PEEKVEC:
                if (opCode == toc_POPVEC && Interpreter_stackPointers[currentStackIndex] <= NumDataFields-1) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Negative stack depth for stack number %d.", currentStackIndex);
                    goto syntax_error;
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
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No predicate.");
                    goto syntax_error;
                }
                if (!intEval) {
                    skipFlag = true;
                }
                break;
            case toc_ZERO:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No parameter to set.");
                    goto syntax_error;
                }
                errNo = os_SetRealVar(paramVar, &Const_Real0);
                if (errNo) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to zero out %c: %d.", paramVar[0], errNo);   
                    goto syntax_error; 
                }
                break;
            case toc_INC:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No parameter to set.");
                    goto syntax_error;
                }
                errNo = os_GetRealVar(paramVar, paramReal);
                if (!errNo) {
                    *paramReal = os_RealAdd(paramReal, getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(paramVar, paramReal);
                } else {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to read %c: %d.", paramVar[0], errNo);
                    goto syntax_error;  
                }
                break;
            case toc_DEC:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No parameter to set.");
                    goto syntax_error;
                }
                errNo = os_GetRealVar(paramVar, paramReal);
                if (!errNo) {
                    *paramReal = os_RealSub(paramReal, getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(paramVar, paramReal);
                } else {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to read %c: %d.", paramVar[0], errNo);
                    goto syntax_error;
                }
                break;
            case toc_STO:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No parameter to set.");
                    goto syntax_error;
                }
                errNo = os_GetRealVar(paramVar, paramReal);
                if (!errNo) {
                    *paramReal = os_RealCopy(getListElementPointerOrDefaultPointer(1, Const_Real1));
                    os_SetRealVar(paramVar, paramReal);
                } else {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to read %c: %d.", paramVar[0], errNo);
                    goto syntax_error;
                }
                break;
            case toc_TURTLE:
                if (intEval < -1 || intEval > NumTurtles) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Invalid turtle number %d.", intEval);
                    goto syntax_error;
                }
                currentTurtleIndex = intEval;
                currentTurtle = &Interpreter_turtles[currentTurtleIndex];
                if (!currentTurtle->Initialized)
                    Turtle_Initialize(currentTurtle);
                break;
            case toc_STACK:
                if (intEval < -1 || intEval > NumStackPages) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Invalid stack number %d.", intEval);
                    goto syntax_error;
                }
                currentStackIndex = intEval;
                break;
            case toc_FADEOUT:
                Palette_FadeOut(Palette_PaletteBuffer, 0, 255, intEval);
                break;
            case toc_FADEIN:
                Palette_FadeIn(Palette_PaletteBuffer, 0, 255, intEval);
                break;
            case toc_PALETTE:
                switch (intEval) {
                    case 0:
                        Palette_Default(Palette_PaletteBuffer);
                        break;
                    case 1:
                        Palette_Rainbow(Palette_PaletteBuffer);
                        break;
                    case 2:
                        Palette_Gray(Palette_PaletteBuffer, intEval);
                        break;
                    default:
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Invalid palette %d.", intEval);
                        goto syntax_error;
                }
                gfx_SetPalette(Palette_PaletteBuffer, 512, 0);
                break;
            case toc_PALSHIFT:
                Palette_Shift(Palette_PaletteBuffer);
                gfx_SetPalette(Palette_PaletteBuffer, 512, 0);
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
                intEval = KeyHelper_GetKey_Inline();
                if (paramReal == NULL) {
                    retList[0] = (float)intEval;
                    retListPointer = 1;
                } else {
                    *paramReal = os_Int24ToReal(intEval);
                    errNo = os_SetRealVar(paramVar, paramReal);
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf(" Wrote %d to %c ", intEval, paramVar[0]);
                    #endif
                    if (errNo) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to write %c: %d.", paramVar[0], errNo);
                        goto syntax_error;
                    }
                }
                break;
            case toc_KEYDOWN:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No key value.");
                    goto syntax_error;
                }
                retList[0] = KeyHelper_IsDown(intEval);
                retListPointer = 1;
                break;
            case toc_IFKEYDOWN:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No key value.");
                    goto syntax_error;
                }
                if (!KeyHelper_IsDown(intEval)) {
                    skipFlag = true;
                }
                break;
            case toc_KEYUP:
                retList[0] = KeyHelper_IsUp(intEval);
                retListPointer = 1;
                break;
            case toc_IFKEYUP:
                if (paramReal == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: No key value.");
                    goto syntax_error;
                }
                if (!KeyHelper_IsUp(intEval)) {
                    skipFlag = true;
                }
                break;
            case toc_KEYSCAN:
                kb_Scan();
                break;
            case toc_SIZESPRITE: {
                if (paramsListLength != 3) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Incorrect number of parameters. Expected 3, got %d.", paramsListLength);
                    goto syntax_error;
                }
                if (intEval < 0 || intEval > NumSprites) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Out of range of sprites %d.", intEval);
                    goto syntax_error;
                }
                currentSpriteIndex = intEval;
                if (Interpreter_spriteDictionary[currentSpriteIndex] != NULL) {
                    #ifdef DEBUG_PROCESSOR
                    dbg_printf("freeing existing sprite [%d]:%p ", intEval, Interpreter_spriteDictionary[currentSpriteIndex]);
                    #endif
                    free(Interpreter_spriteDictionary[currentSpriteIndex]);
                }
                
                uint8_t w = max(255, os_RealToInt24(&paramsList->items[1]));
                uint8_t h = max(255, os_RealToInt24(&paramsList->items[2]));
                #ifdef DEBUG_PROCESSOR
                dbg_printf("allocating %dx%d sprite", w, h);
                #endif
                Interpreter_spriteDictionary[currentSpriteIndex] = gfx_MallocSprite(w, h);
                if (Interpreter_spriteDictionary[currentSpriteIndex] == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Could not allocate memory for sprite %d.", currentSpriteIndex);
                    goto syntax_error;
                }
                #ifdef DEBUG_PROCESSOR
                dbg_printf(": %p ", Interpreter_spriteDictionary[currentSpriteIndex]);
                #endif
                break;}
            case toc_DEFSPRITE:
                if (intEval < 0 || intEval > NumSprites) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Out of range of sprites %d.", intEval);
                    goto syntax_error;
                }
                if (Interpreter_spriteDictionary[currentSpriteIndex] == NULL) {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Sprite %d undefined. You must call SIZESPRITE first.", intEval);
                    goto syntax_error;
                }
                if (params[0] == Token_Flag_NoEvalParams) {
                    Interpreter_copySprite(paramsStringLength - 1, &params[1], Interpreter_spriteDictionary[currentSpriteIndex]);
                } else {
                    snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Only quoted strings are current supported.");
                    goto syntax_error;
                }
                break;
            case toc_ASM:
                if (params[0] != Token_Flag_NoEvalParams) {
                    if (ansString == NULL) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Ans wasn't a string. Type: %d.", type);
                        goto syntax_error;
                    }
                    Interpreter_execAsmString(ansString->len, (const uint8_t*)&ansString->data[0], currentTurtle);
                } else { 
                    Interpreter_execAsmString(paramsStringLength - 1, &params[1], currentTurtle);
                }
                break;
            case toc_TEXT:
                if (params[0] != Token_Flag_NoEvalParams) {
                    if (ansString == NULL) {
                        snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Ans wasn't a string. Type: %d.", type);
                        goto syntax_error;
                    }
                    Interpreter_printString(ansString->len, (const uint8_t*)&ansString->data[0], currentTurtle);
                } else { 
                    Interpreter_printString(paramsStringLength - 1, &params[1], currentTurtle);
                }
                break;
            case toc_ONERROR:
                pauseOnError = intEval != 1;
                break;
            case toc_AUTODRAW:
                autoDraw = intEval != 0;
                #ifdef DEBUG_PROCESSOR
                dbg_printf("Setting autodraw to %d.", autoDraw);
                #endif
                break;
            case toc_SPRITE:
                Turtle_SetSpriteNumber(currentTurtle, intEval);
                #ifdef DEBUG_PROCESSOR
                dbg_printf("Setting sprite number to %d.", intEval);
                #endif
                break;
            case toc_DRAW:
                gfx_SetColor(currentTurtle->Color);
                Turtle_Draw(currentTurtle, Interpreter_spriteDictionary);
                break;
            case toc_UNKNOWN:
                snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Unknown hash encountered 0x%.6lX command %.*s.", (uint32_t)commandHash, commandStringLength, command);
                goto syntax_error;
                break;
            default:
                snprintf(errorMessage, errorMessageLength, "\nUNIMPLEMENETED OPCODE %d %.*s.", opCode, commandStringLength, command);
                goto syntax_error;
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
            snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to set ans dim %d.", retListPointer);
            goto syntax_error;
        }

        for (uint16_t retListIndex = 0; retListIndex < retListPointer; retListIndex++) {
            #ifdef DEBUG_PROCESSOR
            dbg_printf("%.2f ", retList[retListIndex]);
            #endif
            ansEntry = os_FloatToReal(retList[retListIndex]);
            errNo = os_SetRealListElement(OS_VAR_ANS, retListIndex+1, &ansEntry);
            if (errNo) {
                snprintf(errorMessage, errorMessageLength, "SYNTAX ERROR: Got error trying to set ans index %d.", retListIndex);
                goto syntax_error;
            }
        }

        goto end_eval;
syntax_error:
        #ifdef DEBUG
        #ifndef DEBUG_PROCESSOR
        dbg_printf("%.8d: ", lineStartPc);
        debug_print_tokens(command, commandStringLength, NULL);
        dbg_printf(" ");
        debug_print_tokens(params, paramsStringLength, NULL);
        #endif
        dbg_printf("\n\t");
        dbg_printf("%.*s\n", errorMessageLength, errorMessage);
        #endif

        gfx_BlitScreen();
        gfx_SetTextFGColor(124);
        gfx_PrintStringXY("ERROR at ", 4, 4);
        gfx_PrintUInt(lineStartPc, 0);
        gfx_PrintString("- Check console.");
        
        #if 0
        const char* messageBuffer = errorMessage;
        uint16_t y = 14;
        gfx_SetTextXY(4, y);
        uint16_t width = 0;
        while (*messageBuffer != 0 && y < GFX_LCD_HEIGHT) {
            gfx_PrintChar(*messageBuffer);
            width += gfx_GetCharWidth(*messageBuffer);
            if (width >= GFX_LCD_WIDTH) {
                y += 8;
                gfx_SetTextXY(8, y);
            }
            messageBuffer++;
        }
        #endif

        if (!pauseOnError) {
            gfx_PrintString(" Paused.");
            goto end_eval;
        }

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

        if (autoDraw) {
            for (uint8_t i = 0; i < NumTurtles; i++) {
                Turtle_Draw(&Interpreter_turtles[i], Interpreter_spriteDictionary);
            }
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
            snprintf(buffer, 4, "%.3d", (uint8_t)fps);
            gfx_PrintStringXY(buffer, 4, 4);
        }
    }

    dbg_printf("Done. PC: %.8d\n", programCounter);
    gfx_SetDrawScreen();
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
    
    #ifdef DEBUG
    dbg_printf("Cleanup.\n");
    #endif

    for (uint16_t i = 0; i < NumSprites; i++) {
        if (Interpreter_spriteDictionary[i]) {
            free(Interpreter_spriteDictionary[i]);
            Interpreter_spriteDictionary[i] = NULL;
        }
    }

    #ifdef DEBUG
    dbg_printf("Interpreter_Interpret: return\n");
    #endif
}