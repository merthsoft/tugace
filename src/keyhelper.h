#ifndef _KEY_HELPER_H_
#define _KEY_HELPER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t KeyHelper_GetKey(void);
bool KeyHelper_IsDown(int24_t getKey);
bool KeyHelper_IsUp(int24_t getKey);

#ifdef __cplusplus
}
#endif

#endif