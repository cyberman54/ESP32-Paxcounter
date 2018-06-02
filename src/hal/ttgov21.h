// Hardware related definitions for TTGO V2.1 Board

#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define DISPLAY_FLIP  1 // rotated display
#define HAS_LED 23 // green on board LED_G3 (not in initial board version)

// disable brownout detection (needed on TTGOv2 for battery powered operation)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    18 // ESP32 GPIO18 (Pin18) -- HPD13A NSS/SEL (Pin4) SPI Chip Select Input
#define PIN_SPI_MOSI  27 // ESP32 GPIO27 (Pin27) -- HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK   5  // ESP32 GPIO5 (Pin5)   -- HPD13A SCK (Pin5) SPI Clock Input

// non arduino pin definitions
#define RST   LMIC_UNUSED_PIN // connected to ESP32 RST/EN
#define DIO0  26 // ESP32 GPIO26 <-> HPD13A IO0
#define DIO1  33 // ESP32 GPIO33 <-> HPDIO1 <-> HPD13A IO1
#define DIO2  32 // ESP32 GPIO32 <-> HPDIO2 <-> HPD13A IO2

// Hardware pin definitions for TTGO V2 Board with OLED SSD1306 0,96" I2C Display
#define OLED_RST U8X8_PIN_NONE // connected to CPU RST/EN
#define OLED_SDA 21 // ESP32 GPIO4 (Pin4)   -- SD1306 D1+D2
#define OLED_SCL 22 // ESP32 GPIO15 (Pin15) -- SD1306 D0