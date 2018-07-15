// Hardware related definitions for lolin32 (without LoRa shield)

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED NOT_A_PIN // Led os on same pin as Lora SS pin, to avoid problems, we don't use it
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
#define HAS_RGB_LED   13  // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED

#define HAS_SPI   1       // comment out if device shall not send data via SPI

// I2C config for Microchip 24AA02E64 DEVEUI unique address
#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits