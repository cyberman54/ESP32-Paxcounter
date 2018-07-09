// Hardware related definitions for lolin32 lite loraNode32 shield
// See https://github.com/hallard/LoLin32-Lite-Lora

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED       22  // ESP32 GPIO12 (pin22) On Board LED
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
#define HAS_RGB_LED   13  // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED
#define HAS_BUTTON    15  // ESP32 GPIO15 (pin15) Button is on the LoraNode32 shield
#define BUTTON_PULLUP  1  // Button need pullup instead of default pulldown

#define CFG_sx1276_radio 1 // RFM95 module

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS     5 // ESP32  GPIO5 (Pin5)  -- SX1276 NSS  (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  23 // ESP32 GPIO23 (Pin23) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   18 // ESP32 GPIO18 (Pin18  -- SX1276 SCK  (Pin16) SPI Clock Input

// non arduino pin definitions
#define RST   25 // ESP32 GPIO25 (Pin25) -- SX1276 NRESET (Pin7)  Reset Trigger Input
#define DIO0  27 // ESP32 GPIO27 (Pin27) -- SX1276 DIO0   (Pin8)  used by LMIC for detecting LoRa RX_Done & TX_Done
#define DIO1  26 // ESP32 GPIO26 (Pin26) -- SX1276 DIO1   (Pin9)  used by LMIC for detecting LoRa RX_Timeout
#define DIO2  LMIC_UNUSED_PIN // 4 ESP32  GPIO4 (Pin4)  -- SX1276 DIO2   (Pin10) not used by LMIC for LoRa (Timeout for FSK only)
#define DIO5  LMIC_UNUSED_PIN // 35 ESP32 GPIO35 (Pin35) -- SX1276 DIO5   not used by LMIC for LoRa (Timeout for FSK only)

// Hardware pin definitions for LoRaNode32 Board with OLED I2C Display
#define OLED_RST U8X8_PIN_NONE  // Not reset pin
#define OLED_SDA 14             // ESP32 GPIO14 (Pin14) -- OLED SDA
#define OLED_SCL 12             // ESP32 GPIO12 (Pin12) -- OLED SCL

// I2C config for Microchip 24AA02E64 DEVEUI unique address
#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits
