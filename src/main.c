#include <fileioc.h>

#ifdef DEBUG
#include <debug.h>
#endif

#include "interpreter.h"
#include "static.h"

int main(void) {
    const char* filename = "BUSH";
    uint8_t programHandle = ti_OpenVar(filename, "r", OS_TYPE_PRGM);
    if (programHandle == 0) {
        return 1;
    }

    clear_key_buffer();
    
    #ifdef DEBUG
    dbg_ClearConsole();
    dbg_printf("prgm%s - ", filename);
    #endif

    size_t programSize = ti_GetSize(programHandle);
    ProgramToken* program = malloc(programSize);
    ti_Read(program, programSize, 1, programHandle);

    Interpreter_Interpret(program, programSize);

    free(program);
    ti_Close(programHandle);

    return 0;
}