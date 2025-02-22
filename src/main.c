#include <debug.h>
#include <fileioc.h>
#include <graphx.h>
#include <string.h>

#include "const.h"
#include "interpreter.h"
#include "palette.h"
#include "shell.h"
#include "static.h"

#define programBufferSize (65532)
#define varNameBufferSize 10
#define tempVarName "TugaTemp"

int main(void) {
    size_t freeRam = os_MemChk(NULL);
    #ifdef DEBUG
    dbg_printf("\nTUGA\n");
    dbg_printf("Free user RAM before allocating program buffer: %d.\n", os_MemChk(NULL));
    #endif

    uint8_t tempHandle = ti_Open(tempVarName, "r");
    if (tempHandle) {
        ti_Close(tempHandle);
        ti_Delete(tempVarName);
    }

    if (freeRam < programBufferSize) {
        dbg_printf("Not enough free RAM to allocate program buffer. Archive some things. You have %d bytes but need %d bytes.\n", freeRam, programBufferSize);
        return 1;
    }

    ProgramToken* main_programBuffer = (ProgramToken*)os_CreateAppVar(tempVarName, programBufferSize);
    if (main_programBuffer == NULL) {
        dbg_printf("Failed to allocate program buffer.\n");
        return 1;
    }

    #ifdef DEBUG
    dbg_printf("Free user RAM after allocating program buffer: %d.\n", os_MemChk(NULL));
    #endif

    char varNameBuffer[varNameBufferSize];
    uint8_t varType = OS_TYPE_PRGM;
    size_t programSize = 0;
    uint8_t ansStringLength = 0;
    bool showShell = true;
    void* vatPointer = NULL;
    int8_t selectedItemNumber = 0;

    ShellErrorCode shellErrorCode = Shell_GetNameFromAns(varNameBufferSize, varNameBuffer, &ansStringLength);
    if (shellErrorCode == sec_Success) {
        shellErrorCode = Shell_LoadVariable(varNameBufferSize, varNameBuffer, varType, programBufferSize, main_programBuffer, &programSize);
        showShell = shellErrorCode != sec_Success;
    }

    if (!showShell)
        ansStringLength = 0;
    
    gfx_Begin();
    Const_Initialize();

    do {
        if (showShell) {
            clear_key_buffer();
            shellErrorCode = Shell_SelectVariable(vatPointer, varNameBufferSize, varNameBuffer, &varType, &selectedItemNumber);
            if (shellErrorCode == sec_Success) {
                shellErrorCode = Shell_LoadVariable(varNameBufferSize, varNameBuffer, varType, programBufferSize, main_programBuffer, &programSize);
            }
        }

        if (shellErrorCode == sec_Success) {
            Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
            gfx_SetDrawScreen();
            Palette_Default(Palette_PaletteBuffer);
            gfx_FillScreen(0);
            Interpreter_Interpret(programBufferSize, main_programBuffer, programSize);
        } else {
            showShell = false;
        }
    } while (showShell);

    Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
    gfx_SetDrawScreen();
    Palette_Default(Palette_PaletteBuffer);
    gfx_FillScreen(255);
    
    if (ansStringLength > 0) {
        // TODO: Do this without malloc
        string_t* backupString = malloc(sizeof(string_t) + 10);
        backupString->len = ansStringLength;
        strncpy(backupString->data, varNameBuffer, ansStringLength);
        os_CreateString(OS_VAR_ANS, backupString);
        free(backupString);
    }

    ti_Delete(tempVarName);
    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 5);

    gfx_End();
    return 0;
}