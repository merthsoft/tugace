#include <fileioc.h>
#include <string.h>

#ifdef DEBUG
#include <debug.h>
#endif

#include "const.h"
#include "interpreter.h"
#include "static.h"

int main(void) {
    char filename[10];
    uint8_t ansType;
    string_t* ans = os_GetAnsData(&ansType);
    if (ans == NULL || ansType != OS_TYPE_STR) {
        dbg_printf("Ans is not a string. In the future, this will open a file picker.\n");
        return 1;
    }
    dbg_printf("Found string of length %d in ans %s.\n", ans->len, ans->data);

    uint8_t size = ans->len > 8 ? 8 : ans->len;
    strncpy(filename, ans->data, size);
    filename[size] = 0;

    uint8_t programHandle = ti_OpenVar(filename, "r", OS_TYPE_PRGM);
    if (programHandle == 0) {
        dbg_printf("Unable to read prgm%s. In the future, this will open a file picker.\n", filename);
        return 1;
    }

    clear_key_buffer();
    
    #ifdef DEBUG
    dbg_ClearConsole();
    dbg_printf("Reading prgm%s\n", filename);
    #endif

    size_t programSize = ti_GetSize(programHandle);

    if (programSize == 0) {
        dbg_printf("Program size 0!\n");
        ti_Close(programHandle);
        return 2;    
    }

    ProgramToken* program = malloc(programSize);
    if (program == NULL) {
        dbg_printf("Could not allocate program buffer!\n");
        ti_Close(programHandle);
        return 3;
    }
    size_t readCount = ti_Read(program, 1, programSize, programHandle);

    #ifdef DEBUG
    dbg_printf("Size: %d read %d\n", programSize, readCount);
    #endif
    
    if (readCount != programSize) {
        dbg_printf("Size mismatch!\n");
        free(program);
        ti_Close(programHandle);
        return 4;
    }

    Const_Initialize();
    Interpreter_Interpret(program, programSize);

    free(program);
    ti_Close(programHandle);

    string_t* backupString = malloc(sizeof(string_t) + 10);
    backupString->len = size;
    strncpy(backupString->data, filename, size);
    os_CreateString(OS_VAR_ANS, backupString);

    return 0;
}