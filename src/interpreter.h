#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

void Interpreter_Interpret(ProgramToken* program, size_t programSize);

#ifdef __cplusplus
}
#endif

#endif