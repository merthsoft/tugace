#include <string.h>

#include <ti/real.h>
#include <ti/vars.h>

#include "static.h"

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

ProgramCounter Seek_ToLabel(const ProgramToken* data, size_t dataLength, ProgramCounter dataStart, LabelIndex label) {
    if (dataLength == 0) {
        return 0;
    }
    
    size_t index = dataStart;
    while (index < dataLength) {
        do {
            ProgramToken c = data[index];
            if (c == Token_Label || c == Token_LabelOs)
                break;
            if (data[index]        == 'L'
                && data[index + 1] == 'A'
                && data[index + 2] == 'B'
                && data[index + 3] == 'E'
                && data[index + 4] == 'L'
                && data[index + 5] == ' ')
                break;
            
            Seek_ToNewLine(data, dataLength, Token_NewLine, &index);
        } while (index < dataLength);
        
        if (index >= dataLength) {
            return 0;
        }

        index++;
        const ProgramToken* params = &data[index];
        size_t paramsLength = Seek_ToNewLine(data, dataLength, Token_NewLine, &index);

        if (paramsLength == 0) {
            continue;
        }
        
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