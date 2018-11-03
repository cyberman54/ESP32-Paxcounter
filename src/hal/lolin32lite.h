#ifndef _LOLINLITE_H
#define _LOLINLITE_H

#include <stdint.h>

// Hardware related definitions for lolin32lite (without LoRa shield)

#define CFG_sx1272_radio 1 // dummy

#define HAS_LED 22        // on board LED on GPIO22
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW

#endif