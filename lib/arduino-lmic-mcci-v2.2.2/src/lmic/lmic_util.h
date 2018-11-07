/*

Module:  lmic_util.h

Function:
        Declare encoding and decoding utilities for LMIC clients.

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       September 2019

*/

#ifndef _LMIC_UTIL_H_
# define _LMIC_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint16_t LMIC_f2sflt16(float);
uint16_t LMIC_f2sflt12(float);
uint16_t LMIC_f2uflt16(float);
uint16_t LMIC_f2uflt12(float);

#ifdef __cplusplus
}
#endif

#endif /* _LMIC_UTIL_H_ */
