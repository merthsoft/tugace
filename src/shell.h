#ifndef _SHELL_H_
#define _SHELL_H_

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShellErrorCode {
    sec_Success,
    sec_Quit,
    sec_AnsNotValid,
    sec_VarCantBeOpened,
    sec_EmptyVar,
    sec_VarTooBig,
    sec_VarSizeMismatch,
} ShellErrorCode;

ShellErrorCode Shell_GetNameFromAns(uint8_t varNameBufferSize, char varNameBuffer[varNameBufferSize], uint8_t* ansStringLength);

ShellErrorCode Shell_LoadVariable(
    uint8_t varNameBufferSize, 
    char varNameBuffer[varNameBufferSize], 
    uint8_t varType, 
    size_t programBufferSize, 
    ProgramToken programBuffer[programBufferSize], 
    size_t* programSize);

ShellErrorCode Shell_SelectVariable(
    void* varVatPointer, 
    uint8_t varNameBufferSize, 
    char varNameBuffer[varNameBufferSize], 
    uint8_t* varType, 
    int8_t* selectedItemNumber);

#ifdef __cplusplus
}
#endif

#endif