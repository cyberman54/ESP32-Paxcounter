// Hardware related definitions for lolin32lite (without LoRa shield)

#define CFG_sx1272_radio 1 // dummy

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED 22        // on board LED on GPIO22
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW

#define HAS_SPI   1       // comment out if device shall not send data via SPI

// I2C config for Microchip 24AA02E64 DEVEUI unique address
#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits