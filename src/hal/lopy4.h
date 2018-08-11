// Hardware related definitions for Pycom LoPy Board (not: LoPy4)

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1
#define HAS_LED NOT_A_PIN // LoPy4 has no on board LED, so we use RGB LED on LoPy4
#define HAS_RGB_LED   GPIO_NUM_0  // WS2812B RGB LED on GPIO0
#define BOARD_HAS_PSRAM // use extra 4MB extern RAM

// Hardware pin definitions for Pycom LoPy4 board
#define PIN_SPI_SS    GPIO_NUM_18
#define PIN_SPI_MOSI  GPIO_NUM_27
#define PIN_SPI_MISO  GPIO_NUM_19
#define PIN_SPI_SCK   GPIO_NUM_5
#define RST   LMIC_UNUSED_PIN
#define DIO0  GPIO_NUM_23 // LoRa IRQ
#define DIO1  GPIO_NUM_23 // Pin tied via diode to DIO0
#define DIO2  GPIO_NUM_23 // Pin tied via diode to DIO0

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  21      // pin for switching wifi antenna
#define WIFI_ANTENNA 0              // 0 = internal, 1 = external

// !!EXPERIMENTAL - not tested yet!!
// uncomment this only if your LoPy runs on a Pytrack expansion board with GPS
// see http://www.quectel.com/UploadImage/Downlad/Quectel_L76-L_I2C_Application_Note_V1.0.pdf
//#define HAS_GPS 1
//#define GPS_QUECTEL_L76 GPIO_NUM_25, GPIO_NUM_26 // SDA (P22), SCL (P21)
//#define GPS_ADDR 0x10
//#define HAS_BUTTON GPIO_NUM_37 // (P14)
//#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown

// uncomment this only if your LoPy runs on a expansion board 3.0
#define HAS_BATTERY_PROBE ADC1_GPIO39_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BATT_FACTOR 2 // voltage divider 1MOhm/1MOhm on board
#define HAS_BUTTON GPIO_NUM_37 // (P14)
#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown

// uncomment this only if your LoPy runs on a expansion board 2.0
//#define HAS_BATTERY_PROBE ADC1_GPIO39_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BATT_FACTOR 4 // voltage divider 115kOhm/56kOhm on board
//#define HAS_BUTTON GPIO_NUM_13 // (P10)
//#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown