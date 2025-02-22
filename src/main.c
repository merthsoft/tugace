#include <debug.h>
#include <fileioc.h>
#include <graphx.h>
#include <string.h>

#include "const.h"
#include "interpreter.h"
#include "palette.h"
#include "shell.h"
#include "static.h"

#define main_programBufferSize (65532)
#define varNameBufferSize 10
#define TempVarName "TugaTemp"

int main(void) {
    Const_Initialize();

    uint8_t tempHandle = ti_Open(TempVarName, "r");
    if (tempHandle) {
        ti_Close(tempHandle);
        ti_Delete(TempVarName);
    }

    ProgramToken* main_programBuffer = (ProgramToken*)os_CreateAppVar(TempVarName, main_programBufferSize);
    if (main_programBuffer == NULL) {
        dbg_printf("Failed to allocate program buffer.\n");
        return 1;
    }

    char varNameBuffer[varNameBufferSize];
    uint8_t varType = OS_TYPE_PRGM;
    size_t programSize = 0;
    uint8_t ansStringLength = 0;
    bool showShell = true;
    void* vatPointer = NULL;
    int8_t selectedItemNumber = 0;

    ShellErrorCode shellErrorCode = Shell_GetNameFromAns(varNameBufferSize, varNameBuffer, &ansStringLength);
    if (shellErrorCode == sec_Success) {
        shellErrorCode = Shell_LoadVariable(varNameBufferSize, varNameBuffer, varType, main_programBufferSize, main_programBuffer, &programSize);
        showShell = shellErrorCode != sec_Success;
    }

    if (!showShell)
        ansStringLength = 0;
    
    gfx_Begin();

    do {
        if (showShell) {
            clear_key_buffer();
            shellErrorCode = Shell_SelectVariable(vatPointer, varNameBufferSize, varNameBuffer, &varType, &selectedItemNumber);
            if (shellErrorCode == sec_Success) {
                shellErrorCode = Shell_LoadVariable(varNameBufferSize, varNameBuffer, varType, main_programBufferSize, main_programBuffer, &programSize);
            }
        }

        if (shellErrorCode == sec_Success) {
            Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
            gfx_SetDrawScreen();
            Palette_Default(Palette_PaletteBuffer);
            gfx_FillScreen(0);
            Interpreter_Interpret(main_programBufferSize, main_programBuffer, programSize);
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

    ti_Delete(TempVarName);
    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 5);

    gfx_End();
    return 0;
}