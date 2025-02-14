#ifndef _KEY_HELPER_H_
#define _KEY_HELPER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t keyhelper_GetKey(void);
bool keyhelper_IsDown(int24_t getKey);
bool keyhelper_IsUp(int24_t getKey);

#ifdef __cplusplus
}
#endif

#endif