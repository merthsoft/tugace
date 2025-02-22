/* Merthsoft Creations 2025
   _____                                                      _____ 
  ( ___ )                                                    ( ___ )
   |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   | 
   |   | _███████████_█████__█████___█████████____█████████__ |   | 
   |   | ░█░░░███░░░█░░███__░░███___███░░░░░███__███░░░░░███_ |   | 
   |   | ░___░███__░__░███___░███__███_____░░░__░███____░███_ |   | 
   |   | ____░███_____░███___░███_░███__________░███████████_ |   | 
   |   | ____░███_____░███___░███_░███____█████_░███░░░░░███_ |   | 
   |   | ____░███_____░███___░███_░░███__░░███__░███____░███_ |   | 
   |   | ____█████____░░████████___░░█████████__█████___█████ |   | 
   |   | ___░░░░░______░░░░░░░░_____░░░░░░░░░__░░░░░___░░░░░_ |   | 
   |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___| 
  (_____)                                                    (_____) 
*/

#include <debug.h>
#include <fileioc.h>
#include <graphx.h>
#include <string.h>

#include "const.h"
#include "interpreter.h"
#include "palette.h"
#include "shell.h"
#include "static.h"

#define programBufferSize (65536/2)
static ProgramToken main_programBuffer[programBufferSize];
#define varNameBufferSize 10

int main(void) {
    #ifdef DEBUG
    dbg_printf("\nTUGA\n");
    dbg_printf("Free user RAM at start: %d.\\n", os_MemChk(NULL));
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

    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 5);

    gfx_End();
    return 0;
}