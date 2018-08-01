// Hardware related definitions for generic ESP32 boards

#define HAS_LORA 1 // comment out if device shall not send data via LoRa or has no LoRa
#define HAS_SPI  1 // comment out if device shall not send data via SPI

#define CFG_sx1276_radio 1 // select LoRa chip
//#define CFG_sx1272_radio 1 // select LoRa chip
#define BOARD_HAS_PSRAM // use if board has external PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // use if display is rotated
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // uses GPIO7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board

#define HAS_LED GPIO_NUM_21 // on board  LED
#define HAS_BUTTON GPIO_NUM_39 // on board button
#define HAS_RGB_LED   GPIO_NUM_0  // WS2812B RGB LED on GPIO0

#define BOARD_HAS_PSRAM // use extra 4MB extern RAM

#define HAS_GPS 1 // use if board has GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M or 7M with default configuration

// pin definitions for SPI interface of LoRa chip
#define PIN_SPI_SS    GPIO_NUM_18 // SPI Chip Select
#define PIN_SPI_MOSI  GPIO_NUM_27 // SPI Data Input
#define PIN_SPI_MISO  GPIO_NUM_19 // SPI Data Output
#define PIN_SPI_SCK   GPIO_NUM_5  // SPI Clock
#define RST LMIC_UNUSED_PIN  // LoRa Reset (if wired)
#define DIO0 GPIO_NUM_26     // LoRa IO0
#define DIO1 GPIO_NUM_32     // LoRa IO1
#define DIO2 LMIC_UNUSED_PIN // LoRa IO2 (not needed)

// pin definitions for I2C interface of OLED Display
#define OLED_RST GPIO_NUM_16 // SSD1306 RST
#define OLED_SDA GPIO_NUM_4  // SD1306 D1+D2
#define OLED_SCL GPIO_NUM_15 // SD1306 D0