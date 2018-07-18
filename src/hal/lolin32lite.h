// Hardware related definitions for lolin32lite (without LoRa shield)

#define CFG_sx1272_radio 1 // dummy

#define HAS_LED 22        // on board LED on GPIO22
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW

#define HAS_SPI   1       // comment out if device shall not send data via SPI