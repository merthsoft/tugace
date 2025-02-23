#include <string.h>

#include <ti/real.h>
#include <ti/vars.h>

#include "inline.h"
#include "static.h"

__attribute__((hot))
ProgramCounter Seek_ToNewLine(const ProgramToken* data, size_t dataLength, ProgramToken additionalDelim, ProgramCounter* index) {
    const size_t start = *index;

    if (dataLength == 0) {
        return 0;
    }

    if (start > dataLength) {
        return 0;
    }

    const char* d = (char*)data;
    do {
        char c = d[*index];
        *index = *index + 1;
        if (c == Token_NewLine || c == additionalDelim) {
            return *index - start - 1; 
        }
    } while (*index < dataLength);
    
    *index = dataLength;
    return *index - start;
}

__attribute__((hot))
ProgramCounter Seek_ToLabel(size_t dataLength, const ProgramToken data[dataLength], ProgramCounter dataStart, uint24_t labelHash, LabelIndex label) {
    size_t index = dataStart;
    while (index < dataLength) {
        do {
            ProgramToken c = data[index];
            while (c == Token_Indent) {
                c = data[++index];
            }

            if (c == Shorthand_Label || c == Shorthand_LabelOs) {
                index++;
                break;
            }

            if (data[index]        == OS_TOK_L
                && data[index + 1] == OS_TOK_A
                && data[index + 2] == OS_TOK_B
                && data[index + 3] == OS_TOK_E
                && data[index + 4] == OS_TOK_L
                && data[index + 5] == OS_TOK_SPACE) {
                index += 6;
                break;
            }
            
            Seek_ToNewLine(data, dataLength, Token_NewLine, &index);
        } while (index < dataLength);
        
        if (index >= dataLength) {
            return 0;
        }

        const ProgramToken* params = &data[index];
        size_t paramsLength = Seek_ToNewLine(data, dataLength, Token_NewLine, &index);

        if (paramsLength == 0) {
            continue;
        }
        
        if (labelHash && params[0] != Token_Flag_EvalParams) {
            uint24_t hash = Hash_InLine(params, paramsLength);
            if (hash == labelHash)
                return index;
            continue;
        } 

        if (labelHash != 0)
            continue;

        if (os_Eval(params, paramsLength)) {
            continue;
        }

        uint8_t type;
        void* ans = os_GetAnsData(&type);
        if (!ans) {
            continue;
        }
        real_t* realParam;
        uint24_t param;
        switch (type) {
            case OS_TYPE_REAL:
                realParam = ans;
                break;
            case OS_TYPE_CPLX:
                realParam = ans;
                break;
            default:
                continue;
        }
        param = os_RealToInt24(realParam);
        if (param == label)
            return index;
    }

    return 0;
}