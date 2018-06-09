// Hardware related definitions for Pycom LoPy Board (not: LoPy4)

#define CFG_sx1272_radio 1
#define HAS_LED NOT_A_PIN // LoPy4 has no on board LED, so we use RGB LED on LoPy4
#define HAS_RGB_LED GPIO_NUM_0  // WS2812B RGB LED on GPIO0

// use only if your LoPy lives on a Pytrack expansion board
//#define HAS_GPS 1
//#define GPS_I2C GPIO_NUM_9, GPIO_NUM_8 // SDA, SCL
//#define HAS_BUTTON GPIO_NUM_4

// Hardware pin definitions for Pycom LoPy board
#define PIN_SPI_SS    GPIO_NUM_17
#define PIN_SPI_MOSI  GPIO_NUM_27
#define PIN_SPI_MISO  GPIO_NUM_19
#define PIN_SPI_SCK   GPIO_NUM_5
#define RST   GPIO_NUM_18
#define DIO0  GPIO_NUM_23 // LoRa IRQ
#define DIO1  GPIO_NUM_23 // workaround
#define DIO2  LMIC_UNUSED_PIN

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  16      // pin for switching wifi antenna
#define WIFI_ANTENNA 0              // 0 = internal, 1 = external
