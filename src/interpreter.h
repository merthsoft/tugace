#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "static.h"

#ifdef __cplusplus
extern "C" {
#endif

void Interpreter_Interpret(size_t programSize, ProgramToken program[programSize]);

#ifdef __cplusplus
}
#endif

#endif