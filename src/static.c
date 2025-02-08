#include <graphx.h>

#include "static.h"

real_t StaticReal_0;
real_t StaticReal_1;

void Static_Initialize(void) {
    StaticReal_0 = os_Int24ToReal(0);
    StaticReal_1 = os_Int24ToReal(1);
}