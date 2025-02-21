#include <debug.h>

#include <fileioc.h>
#include <string.h>

#include <ti/vars.h>

#include "palette.h"
#include "shell.h"

ShellErrorCode Shell_GetNameFromAns(uint8_t varNameBufferSize, char varNameBuffer[varNameBufferSize], uint8_t* ansStringLength) {
    uint8_t ansType;
    string_t* ans = os_GetAnsData(&ansType);
    if (ans == NULL || ansType != OS_TYPE_STR) {
        return sec_AnsNotValid;    
    }

    if (ans->len > varNameBufferSize-1) {
        return sec_AnsNotValid;
    }
    
    *ansStringLength = ans->len;
    strncpy(varNameBuffer, ans->data, *ansStringLength);
    varNameBuffer[*ansStringLength] = 0;
    return sec_Success;
}

ShellErrorCode Shell_LoadVariable(uint8_t varNameBufferSize, char varNameBuffer[varNameBufferSize], uint8_t varType, size_t programBufferSize, ProgramToken programBuffer[programBufferSize], size_t* programSize) {
    uint8_t programHandle = ti_OpenVar(varNameBuffer, "r", varType);
    // Fallback for if loaded from ans
    if (programHandle == 0 && varType == OS_TYPE_PRGM) {
        programHandle = ti_OpenVar(varNameBuffer, "r", OS_TYPE_PROT_PRGM);
    }

    if (programHandle == 0) {
        return sec_VarCantBeOpened;
    }

    *programSize = ti_GetSize(programHandle);

    if (*programSize == 0) {
        ti_Close(programHandle);
        return sec_EmptyVar;
    }

    if (*programSize > programBufferSize) {
        ti_Close(programHandle);
        return sec_VarTooBig;
    }

    size_t readCount = ti_Read(programBuffer, 1, *programSize, programHandle);
    ti_Close(programHandle);
    
    if (readCount != *programSize) {
        return sec_VarSizeMismatch;
    }

    return sec_Success;
}

ShellErrorCode Shell_SelectVariable(void *varVatPointer, uint8_t varNameBufferSize, char varNameBuffer[varNameBufferSize], uint8_t *varType, uint8_t *selectedItemNumber) {
    char* varName;
    varNameBuffer[0] = 0;
    *selectedItemNumber = 0;
    
    Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 5);
    gfx_SetDrawBuffer();

    gfx_FillScreen(2);
    gfx_SetTextScale(3, 3);
    gfx_SetTextFGColor(23);
    gfx_PrintStringXY("TUGA CE v0.2a", 2, 2);

    #define ShellRectStartX 2
    #define ShellRectStartY 28
    #define ShellRectWidth GFX_LCD_WIDTH - (ShellRectStartX + 2)
    #define ShellRectHeight GFX_LCD_HEIGHT - (28 + 2)

    #define ShellIconSize 70
    #define ShellIconLabelOffet 62

    #define ShellIconStartX 4
    #define ShellIconStartY 30

    gfx_SetColor(0);
    gfx_Rectangle(ShellRectStartX, ShellRectStartY, ShellRectWidth, ShellRectHeight);
    gfx_SwapDraw();
    
    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 5);
    
    bool exit = false;
    while (!exit) {
        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);

        gfx_BlitScreen();
        gfx_SetColor(2);
        gfx_FillRectangle_NoClip(ShellRectStartX + 1, ShellRectStartY + 1, ShellRectWidth - 2, ShellRectHeight - 2);

        uint24_t textX = ShellIconStartX;
        uint24_t textY = ShellIconStartY;
        gfx_SetTextFGColor(0);
        gfx_SetTextScale(1, 2);
        
        void* trackingPointer = varVatPointer;
        while ((varName = ti_DetectAny(&trackingPointer, "TUGA", varType)) && textY < GFX_LCD_HEIGHT - 10) {
            if (*varType == OS_TYPE_PRGM || *varType == OS_TYPE_PROT_PRGM || *varType == OS_TYPE_APPVAR) {
                gfx_SetTextXY(textX, textY + ShellIconLabelOffet);
                gfx_PrintString(varName);
                textX += ShellIconSize;
                if (textX >= ShellIconSize * 3) {
                    textX = ShellIconStartX;
                    textY += ShellIconSize;
                }
            }
        }

        gfx_SwapDraw();
    }

    return sec_Quit;
}