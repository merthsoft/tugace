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

#define programBufferSize (OS_VAR_MAX_SIZE-5)
#define varNameBufferSize 10
#define tempVarName "TugaTemp"

uint8_t main_getTempAppVar() {
    if (os_ChkFindSym(OS_TYPE_APPVAR, tempVarName, NULL, NULL)) {
        ti_Delete(tempVarName);
    }

    if (!os_CreateAppVar(tempVarName, programBufferSize)) {
        return 0;
    }
    return ti_Open(tempVarName, "r+");
}

ProgramToken* main_lockVar(bool archiveStatus, uint8_t handle) {
    if (!ti_SetArchiveStatus(archiveStatus, handle)) {
        return NULL;
    }
    ti_Close(handle);
    ti_Open(tempVarName, archiveStatus ? "r" : "r+");
    return ti_GetDataPtr(handle);
}

int main(void) {
    #ifdef DEBUG
    dbg_printf("\nTUGA START\n");
    dbg_printf("Free user RAM: %d.\n", os_MemChk(NULL));
    #endif

    char varNameBuffer[varNameBufferSize];
    uint8_t varType = OS_TYPE_PRGM;
    size_t programSize = 0;
    uint8_t ansStringLength = 0;
    bool showShell = false;
    void* vatPointer = NULL;
    int8_t selectedItemNumber = 0;
    
    ProgramToken* programBuffer = NULL;
    uint8_t programBufferHandle = main_getTempAppVar();
    if (programBufferHandle == 0) {
        dbg_printf("Couldn't open appvar.\n");
        return 1;
    }
    
    gfx_Begin();
    Const_Initialize();
    
    ShellErrorCode shellErrorCode = Shell_GetNameFromAns(varNameBufferSize, varNameBuffer, &ansStringLength);
    if (shellErrorCode != sec_Success) {
        showShell = true;
    }

    do {
        clear_key_buffer();

        if (showShell) {
            shellErrorCode = Shell_SelectVariable(vatPointer, varNameBufferSize, varNameBuffer, &varType, &selectedItemNumber);
            if (shellErrorCode == sec_Quit)
                break;
            
            if (shellErrorCode != sec_Success) {
                dbg_printf("Error selecting variable from shell: %d.", shellErrorCode);
                break;
            }
        }
        
        programBuffer = ti_GetDataPtr(programBufferHandle);
        shellErrorCode = Shell_LoadVariable(varNameBufferSize, varNameBuffer, varType, programBufferSize, programBuffer, &programSize);

        if (shellErrorCode != sec_Success) {
            dbg_printf("Error loading variable from shell: %d.", shellErrorCode);
            break;
        }
        Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
        gfx_SetDrawScreen();
        Palette_Default(Palette_PaletteBuffer);
        gfx_FillScreen(0);
        Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 20);
        clear_key_buffer();

        programBuffer = main_lockVar(true, programBufferHandle);
        if (programBuffer == NULL) {
            dbg_printf("Couldn't lock appvar.\n");
            break;
        }
        
        Interpreter_Interpret(programBufferSize, programBuffer, programSize);
        programBuffer = main_lockVar(false, programBufferHandle);
        
        if (programBuffer == NULL) {
            dbg_printf("Couldn't get appvar data pointer.\n");
            break;
        }
        
        // Delete the header from the appvar so it doesn't show up in the shell
        programBuffer[0] = 0;        
    } while (showShell);

    Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
    gfx_SetDrawScreen();
    Palette_Default(Palette_PaletteBuffer);
    gfx_FillScreen(255);
    
    if (ansStringLength > 0) {
        // TODO: Do this without malloc
        string_t* backupString = ti_MallocString(ansStringLength);
        strncpy(backupString->data, varNameBuffer, ansStringLength);
        os_CreateString(OS_VAR_ANS, backupString);
        free(backupString);
    }

    ti_Close(programBufferHandle);
    ti_Delete(tempVarName);

    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 5);

    gfx_End();

    #ifdef DEBUG
    dbg_printf("\nFree user RAM: %d.", os_MemChk(NULL));
    dbg_printf("\nTUGA END\n");
    #endif

    return 0;
}