/*

Module:  header_test.ino

Function:
        Simple hello-world (and compile-test) app

Copyright notice and License:
        See LICENSE file accompanying this project.

Author:
        Terry Moore, MCCI Corporation	April 2018

*/

#include <lmic.h>

# define STATIC_ASSERT(e)	\
 void  STATIC_ASSERT__(int MCCIADK_C_ASSERT_x[(e) ? 1: -1])

STATIC_ASSERT(ARDUINO_LMIC_VERSION >= ARDUINO_LMIC_VERSION_CALC(2,1,5,0));

STATIC_ASSERT(ARDUINO_LMIC_VERSION_CALC(1,2,3,4) == 0x01020304);

STATIC_ASSERT(ARDUINO_LMIC_VERSION_GET_MAJOR(ARDUINO_LMIC_VERSION_CALC(1,2,3,4)) == 1);
STATIC_ASSERT(ARDUINO_LMIC_VERSION_GET_MINOR(ARDUINO_LMIC_VERSION_CALC(1,2,3,4)) == 2);
STATIC_ASSERT(ARDUINO_LMIC_VERSION_GET_PATCH(ARDUINO_LMIC_VERSION_CALC(1,2,3,4)) == 3);
STATIC_ASSERT(ARDUINO_LMIC_VERSION_GET_LOCAL(ARDUINO_LMIC_VERSION_CALC(1,2,3,4)) == 4);

void setup()
	{
	}

void loop()
	{
	}
