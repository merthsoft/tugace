#ifndef _SEEK_H_
#define _SEEK_H_

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

ProgramCounter Seek_ToLabel(
    size_t dataLength, 
    const ProgramToken data[dataLength], 
    ProgramCounter dataStart,
    uint24_t labelHash,
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