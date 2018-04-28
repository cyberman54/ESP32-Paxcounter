// Hardware related definitions for Heltec LoRa-32 Board

#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED GPIO_NUM_25 // white LED on board
#define HAS_BUTTON GPIO_NUM_0 // button "PROG" on board

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    18 // ESP32 GPIO18 (Pin18) -- SX1276 NSS (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  27 // ESP32 GPIO27 (Pin27) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   5  // ESP32 GPIO5 (Pin5)   -- SX1276 SCK (Pin16) SPI Clock Input

// non arduino pin definitions
#define RST   14 // ESP32 GPIO14 (Pin14) -- SX1276 NRESET (Pin7) Reset Trigger Input
#define DIO0  26 // ESP32 GPIO26 (Pin15) -- SX1276 DIO0 (Pin8) used by LMIC for detecting LoRa RX_Done & TX_Done
#define DIO1  33 // ESP32 GPIO33 (Pin13) -- SX1276 DIO1 (Pin9) used by LMIC for detecting LoRa RX_Timeout
#define DIO2  LMIC_UNUSED_PIN // 32 ESP32 GPIO32 (Pin12) -- SX1276 DIO2 (Pin10) not used by LMIC for LoRa (Timeout for FSK only)

// Hardware pin definitions for Heltec LoRa-32 Board with OLED SSD1306 I2C Display
#define OLED_RST 16 // ESP32 GPIO16 (Pin16) -- SD1306 RST
#define OLED_SDA 4  // ESP32 GPIO4 (Pin4)   -- SD1306 D1+D2
#define OLED_SCL 15 // ESP32 GPIO15 (Pin15) -- SD1306 D0
