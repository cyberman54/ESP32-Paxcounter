// clang-format off
// upload_speed 921600
// board lolin32

#ifndef _LOLINLITE_H
#define _LOLINLITE_H

#include <stdint.h>

// Hardware related definitions for lolin32lite (without LoRa shield)

#define HAS_LED LED_BUILTIN // on board LED on GPIO5
#define LED_ACTIVE_LOW 1    // Onboard LED is active when pin is LOW

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif