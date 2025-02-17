#include "keyhelper.h"

#include <debug.h>
#include <keypadc.h>

#define keyhelper_scanCodeToGetKeyLutOffset 1
#define keyhelper_scanCodeToGetKeyLutSize   56
static const uint8_t keyhelper_scanCodeToGetKeyLut[keyhelper_scanCodeToGetKeyLutSize] = {
    34,     // skDown		equ 01h
    24,     // skLeft		equ 02h
    26,     // skRight		equ 03h
    25,     // skUp			equ 04h
    0,      // 05h 
    0,      // 06h
    0,      // 07h
    0,      // 08h
    105,    // skEnter		equ 09h
    95,     // skAdd		equ 0Ah
    85,     // skSub		equ 0Bh
    75,     // skMul		equ 0Ch
    65,     // skDiv		equ 0Dh
    55,     // skPower		equ 0Eh
    45,     // skClear		equ 0Fh
    0,      // 10h
    104,    // skChs		equ 11h
    94,     // sk3			equ 12h
    84,     // sk6			equ 13h
    74,     // sk9			equ 14h
    64,     // skRParen		equ 15h
    54,     // skTan		equ 16h
    44,     // skVars		equ 17h
    0,      // 18h
    103,    // skDecPnt		equ 19h
    93,     // sk2			equ 1Ah
    83,     // sk5			equ 1Bh
    73,     // sk8			equ 1Ch
    63,     // skLParen		equ 1Dh
    53,     // skCos		equ 1Eh
    43,     // skPrgm		equ 1Fh
    33,     // skStat		equ 20h
    102,    // sk0			equ 21h
    92,     // sk1			equ 22h
    82,     // sk4			equ 23h
    72,     // sk7			equ 24h
    62,     // skComma		equ 25h
    52,     // skSin		equ 26h
    42,     // skMatrix		equ 27h
    32,     // skGraphvar	equ 28h
    0,      // 29h
    91,     // skStore		equ 2Ah
    81,     // skLn			equ 2Bh
    71,     // skLog		equ 2Ch
    61,     // skSquare		equ 2Dh
    51,     // skRecip		equ 2Eh
    41,     // skMath		equ 2Fh
    31,     // skAlpha		equ 30h
    15,     // skGraph		equ 31h
    14,     // skTrace		equ 32h
    13,     // skZoom		equ 33h
    12,     // skWindow		equ 34h
    11,     // skYequ 		equ 35h
    21,     // sk2nd		equ 36h
    22,     // skMode		equ 37h
    23,     // skDel		equ 38h
};

#define keyhelper_getKeyToKbKeyLutOffset 11
#define keyhelper_getKeyToKbKeyLutSize   95
static const kb_lkey_t keyhelper_getKeyToKbKeyLut[keyhelper_getKeyToKbKeyLutSize] = {
    kb_KeyYequ,     // 11
    kb_KeyWindow,
    kb_KeyZoom,
    kb_KeyTrace,
    kb_KeyGraph,
    0,              // 16
    0,              // 17
    0,              // 18
    0,              // 19
    0,              // 20
    kb_Key2nd,
    kb_KeyMode,   
    kb_KeyDel,    
    kb_KeyLeft,   
    kb_KeyUp,    
    kb_KeyRight,  
    0,              // 27
    0,              // 28
    0,              // 29
    0,              // 30
    kb_KeyAlpha,
    kb_KeyGraphVar,
    kb_KeyStat,
    kb_KeyDown,
    0,              // 35
    0,              // 36
    0,              // 37
    0,              // 38
    0,              // 39
    0,              // 40
    kb_KeyMath,
    kb_KeyRecip,
    kb_KeyPrgm,
    kb_KeyVars,
    kb_KeyClear,
    0,              // 46
    0,              // 47
    0,              // 48
    0,              // 49
    0,              // 50
    kb_KeyRecip,
    kb_KeySin,
    kb_KeyCos,
    kb_KeyTan,
    kb_KeyPower,
    0,              // 56
    0,              // 57
    0,              // 58
    0,              // 59
    0,              // 60
    kb_KeySquare,
    kb_KeyComma,
    kb_KeyLParen,
    kb_KeyRParen,
    kb_KeyDiv,
    0,              // 66
    0,              // 67
    0,              // 68
    0,              // 69
    0,              // 70
    kb_KeyLog,
    kb_Key7,
    kb_Key8,
    kb_Key9,
    kb_KeyMul,
    0,              // 76
    0,              // 77
    0,              // 78
    0,              // 79
    0,              // 80
    kb_KeyLn,
    kb_Key4,
    kb_Key5,
    kb_Key6,
    kb_KeySub,
    0,              // 86
    0,              // 87
    0,              // 88
    0,              // 89
    0,              // 90
    kb_KeySto,
    kb_Key1,
    kb_Key2,
    kb_Key3,
    kb_KeyAdd,
    0,              // 96
    0,              // 97
    0,              // 98
    0,              // 99
    0,              // 100
    0,              // 101
    kb_Key0,
    kb_KeyDecPnt,
    kb_KeyChs,
    kb_KeyEnter,
};

uint8_t KeyHelper_GetKey(void) {
    kb_Scan();
    for (uint8_t key = 1, group = 7; group; --group) {
        for (uint8_t mask = 1; mask; mask <<= 1, ++key) {
            if (kb_Data[group] & mask) {
                return keyhelper_scanCodeToGetKeyLut[key - keyhelper_scanCodeToGetKeyLutOffset];
            }
        }
    }

    return 0;
}

bool KeyHelper_IsDown(int24_t getKey) {
    getKey -= keyhelper_getKeyToKbKeyLutOffset;
    if (getKey < 0 || getKey > keyhelper_getKeyToKbKeyLutSize)
        return false;
    return kb_IsDown(keyhelper_getKeyToKbKeyLut[getKey]);
}

bool KeyHelper_IsUp(int24_t getKey) {
    getKey -= keyhelper_getKeyToKbKeyLutOffset;
    if (getKey < 0 || getKey > keyhelper_getKeyToKbKeyLutSize)
        return false;
    return !kb_IsDown(keyhelper_getKeyToKbKeyLut[getKey]);
}