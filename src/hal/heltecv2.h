// Hardware related definitions for Heltec V2 LoRa-32 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED GPIO_NUM_25 // white LED on board
#define HAS_BUTTON GPIO_NUM_0 // button "PROG" on board

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    GPIO_NUM_18 // ESP32 GPIO18 -- SX1276 NSS (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  GPIO_NUM_27 // ESP32 GPIO27 -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  GPIO_NUM_19 // ESP32 GPIO19 -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   GPIO_NUM_5  // ESP32 GPIO5  -- SX1276 SCK (Pin16) SPI Clock Input

// non arduino pin definitions
#define RST   GPIO_NUM_14 // ESP32 GPIO18 -- SX1276 NRESET (Pin7) Reset Trigger Input
#define DIO0  GPIO_NUM_26 // ESP32 GPIO26 -- SX1276 DIO0 (Pin8) used by LMIC for detecting LoRa RX_Done & TX_Done
#define DIO1  GPIO_NUM_34 // ESP32 GPIO33 -- SX1276 DIO1 (Pin9) used by LMIC for detecting LoRa RX_Timeout
#define DIO2  GPIO_NUM_35 // 32 ESP32 GPIO32 -- SX1276 DIO2 (Pin10) not used by LMIC for LoRa (Timeout for FSK only)

// Hardware pin definitions for Heltec LoRa-32 Board with OLED SSD1306 I2C Display
#define OLED_RST GPIO_NUM_16 // ESP32 GPIO16 -- SD1306 RST
#define I2C_SDA GPIO_NUM_4  // ESP32 GPIO4   -- SD1306 D1+D2
#define I2C_SCL GPIO_NUM_15 // ESP32 GPIO15  -- SD1306 D0