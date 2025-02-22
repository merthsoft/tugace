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

ShellErrorCode Shell_SelectVariable(void *varVatPointer, uint8_t varNameBufferSize, char varNameBuffer[varNameBufferSize], uint8_t *varType, int8_t *selectedItemNumber) {
    char* varName;
    varNameBuffer[0] = 0;
    *selectedItemNumber = 0;
    
    Palette_FadeOut(Palette_PaletteBuffer, 0, 255, 20);
    Palette_Default(Palette_PaletteBuffer);
    gfx_SetDrawBuffer();

    #define ShellBackgroundColor 0
    #define ShellLogoColor 254
    #define ShellTextColor 0xDF
    #define ShellHighlightColor 0x2B

    gfx_SetTextScale(3, 3);
    gfx_FillScreen(ShellBackgroundColor);
    gfx_SetTextTransparentColor(ShellBackgroundColor);
    gfx_SetTextBGColor(ShellBackgroundColor);
    gfx_SetTextFGColor(ShellLogoColor);
    gfx_PrintStringXY("TUGA CE v0.2a", 2, 2);

    #define ShellIconSize 79
    #define ShellIconLabelOffet 62

    #define ShellRectStartX 1
    #define ShellRectStartY 28
    #define ShellRectWidth (GFX_LCD_WIDTH - ShellRectStartX)
    #define ShellRectHeight (ShellIconSize*2+3)

    #define ShellIconStartX 2
    #define ShellIconStartY 30

    #define ShellDescriptionStartX 1
    #define ShellDescriptionStartY (ShellRectStartY + ShellRectHeight + 1)

    gfx_SetColor(ShellLogoColor);
    gfx_Rectangle(ShellRectStartX, ShellRectStartY, ShellRectWidth, ShellRectHeight);
    gfx_SwapDraw();
    
    Palette_FadeIn(Palette_PaletteBuffer, 0, 255, 20);
    
    bool exit = false;
    if (*selectedItemNumber > 8 || *selectedItemNumber == 0) {
        *selectedItemNumber = 1;
    }
    
    kb_lkey_t keyDown = 0;
    uint8_t maxItems = 0;
    while (!exit) {
        kb_Scan();
        exit |= kb_IsDown(kb_KeyDel) | kb_IsDown(kb_KeyClear) | kb_IsDown(kb_KeyMode);

        if (!keyDown) {
            if (kb_IsDown(kb_KeyLeft)) {
                keyDown = kb_KeyLeft;
                *selectedItemNumber -= 1;
            } else if (kb_IsDown(kb_KeyRight)) {
                keyDown = kb_KeyRight;
                *selectedItemNumber += 1;
            } else if (kb_IsDown(kb_KeyUp)) {
                keyDown = kb_KeyUp;
                *selectedItemNumber -= 4;
            } else if (kb_IsDown(kb_KeyDown)) {
                keyDown = kb_KeyDown;
                *selectedItemNumber += 4;
            }
        } else {
            if (!kb_IsDown(keyDown))
                keyDown = 0;
        }

        if (*selectedItemNumber == 0)
            *selectedItemNumber = maxItems;
        if (*selectedItemNumber > maxItems || *selectedItemNumber < 0)
            *selectedItemNumber = *selectedItemNumber % maxItems;
        
        gfx_BlitScreen();
        gfx_SetColor(ShellBackgroundColor);
        gfx_FillRectangle_NoClip(ShellRectStartX + 1, ShellRectStartY + 1, ShellRectWidth - 2, ShellRectHeight - 2);
        gfx_SetColor(ShellHighlightColor);
        gfx_FillRectangle_NoClip(ShellDescriptionStartX, ShellDescriptionStartY, GFX_LCD_WIDTH, GFX_LCD_HEIGHT - ShellDescriptionStartY);

        uint24_t textX = ShellIconStartX;
        uint24_t textY = ShellIconStartY;
        gfx_SetTextFGColor(ShellTextColor);
        gfx_SetTextScale(1, 2);
        gfx_SetColor(ShellHighlightColor);

        void* trackingPointer = varVatPointer;
        uint8_t keyIndex = 1;
        maxItems = 0;
        while ((varName = ti_DetectAny(&trackingPointer, "0TUGA", varType)) && maxItems < 8) {
            if (*varType == OS_TYPE_PRGM || *varType == OS_TYPE_PROT_PRGM || *varType == OS_TYPE_APPVAR) {
                maxItems++;
                if (keyIndex == *selectedItemNumber) {
                    strncpy(varNameBuffer, varName, varNameBufferSize);
                    if (kb_IsDown(kb_KeyEnter) || kb_IsDown(kb_2nd)) {
                        return sec_Success;
                    }

                    gfx_FillRectangle_NoClip(textX, textY, ShellIconSize, ShellIconSize);

                    uint8_t handle = ti_OpenVar(varName, "r", *varType);
                    unsigned char c[2] = {0, 0};
                    if (handle) {
                        bool lineFlag = false;
                        while (lineFlag < 1 && ti_Read(c, 1, 1, handle)) {
                            if (c[0] == Token_NewLine)
                                lineFlag = true;
                        }

                        if (lineFlag) {
                            if (ti_Read(c, 1, 1, handle) && c[0] == Token_Header_SpritePrefix) {
                                lineFlag = false;
                                while (!lineFlag && ti_Read(c, 1, 1, handle)) {
                                    if (c[0] == Token_NewLine)
                                        lineFlag = true;
                                }
                                if (!ti_Read(c, 1, 1, handle))
                                    c[0] = 0;
                            }
                            
                            if (c[0] == Token_Header_DescPrefix) {
                                gfx_SetTextXY(ShellDescriptionStartX + 1, ShellDescriptionStartY + 1);
                                lineFlag = false;
                                while (!lineFlag && ti_Read(c, 1, 1, handle)) {
                                    if (*c == OS_TOK_2BYTE || *c == OS_TOK_2BYTE_EXT)
                                        ti_Read(&c[1], 1, 1, handle);
                                    void* ptr = &c;
                                    
                                    if (c[0] == Token_NewLine)
                                        lineFlag = true;
                                    else if (gfx_GetTextY() >= GFX_LCD_WIDTH - 8)
                                        lineFlag = true;
                                    else
                                        gfx_PrintString(ti_GetTokenString(&ptr, NULL, NULL));
                                }
                            }
                        }

                        ti_Close(handle);
                    }
                }

                gfx_SetTextXY(textX, textY + ShellIconLabelOffet);
                gfx_PrintInt(keyIndex++, 1);
                gfx_PrintString(" ");
                gfx_PrintString(varName);
                textX += ShellIconSize;
                if (textX >= ShellIconSize * 4) {
                    textX = ShellIconStartX;
                    textY += ShellIconSize;
                }
            }
        }

        gfx_SwapDraw();
    }

    return sec_Quit;
}