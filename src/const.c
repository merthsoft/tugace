#include "const.h"

real_t Const_Real0;
real_t Const_Real1;

void Const_Initialize(void) {
    Const_Real0 = os_Int24ToReal(0);
    Const_Real1 = os_Int24ToReal(1);
}