// Hardware related definitions for #IoT Octopus32 with the Adafruit LoRaWAN Wing
// You can use this configuration also with the Adafruit ESP32 Feather + the LoRaWAN Wing
// In this config we use the Adafruit OLED Wing which is only 128x32 pixel, need to find a smaller font

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED        13  // ESP32 GPIO12 (pin22) On Board LED
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
//#define HAS_RGB_LED   13  // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED
//#define HAS_BUTTON    15  // ESP32 GPIO15 (pin15) Button is on the LoraNode32 shield
//#define BUTTON_PULLUP  1  // Button need pullup instead of default pulldown

#define HAS_LORA  1       // comment out if device shall not send data via LoRa
#define HAS_SPI   1       // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1 // RFM95 module

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    14 //14 // ESP32  GPIO5 (Pin5)  -- SX1276 NSS  (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  18 // ESP32 GPIO23 (Pin23) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   5 // ESP32 GPIO18 (Pin18) -- SX1276 SCK  (Pin16) SPI Clock Input

//GPIO_NUM_
// non arduino pin definitions
#define RST   LMIC_UNUSED_PIN // ESP32 GPIO25 (Pin25) -- SX1276 NRESET (Pin7)  Reset Trigger Input
#define DIO0  33 // ESP32 GPIO27 (Pin27) -- SX1276 DIO0   (Pin8)  used by LMIC for detecting LoRa RX_Done & TX_Done
#define DIO1  33 // ESP32 GPIO26 (Pin26) -- SX1276 DIO1   (Pin9)  used by LMIC for detecting LoRa RX_Timeout
#define DIO2  LMIC_UNUSED_PIN // 4 ESP32  GPIO4 (Pin4)  -- SX1276 DIO2   (Pin10) not used by LMIC for LoRa (Timeout for FSK only)
#define DIO5  LMIC_UNUSED_PIN // 35 ESP32 GPIO35 (Pin35) -- SX1276 DIO5   not used by LMIC for LoRa (Timeout for FSK only)

// Hardware pin definitions for LoRaNode32 Board with OLED I2C Display
#define OLED_RST U8X8_PIN_NONE  // Not reset pin
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // U8X8_SSD1306_128X32_UNIVISION_SW_I2C // 
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define I2C_SDA 23  //21            // ESP32 GPIO14 (Pin14) -- OLED SDA
#define I2C_SCL 22  //22            // ESP32 GPIO12 (Pin12) -- OLED SCL

// I2C config for Microchip 24AA02E64 DEVEUI unique address
//#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
//#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits
