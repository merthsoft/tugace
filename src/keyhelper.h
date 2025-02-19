#ifndef _KEY_HELPER_H_
#define _KEY_HELPER_H_
#include <keypadc.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool KeyHelper_IsDown(int24_t getKey);
bool KeyHelper_IsUp(int24_t getKey);


#define keyhelper_scanCodeToGetKeyLutOffset 1
#define keyhelper_scanCodeToGetKeyLutSize   56
extern const uint8_t keyhelper_scanCodeToGetKeyLut[keyhelper_scanCodeToGetKeyLutSize];

__attribute__((hot))
static inline uint8_t KeyHelper_GetKey(void) {
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

#ifdef __cplusplus
}
#endif

#endif