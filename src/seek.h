#ifndef _SEEK_H_
#define _SEEK_H_

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

ProgramCounter Seek_ToLabel(
    const ProgramToken* data, 
    size_t dataLength, 
    ProgramCounter dataStart, 
    LabelIndex label);

ProgramCounter Seek_ToNewLine(
    const ProgramToken* data, 
    size_t dataLength, 
    ProgramToken additionalDelim, 
    ProgramCounter* index);

#ifdef __cplusplus
}
#endif

#endif