// Hardware related definitions for Pycom LoPy Board (not: LoPy4)

#define CFG_sx1276_radio 1
#define HAS_LED NOT_A_PIN // LoPy4 has no on board LED, so we use RGB LED on LoPy4
#define HAS_RGB_LED   GPIO_NUM_0  // WS2812B RGB LED on GPIO0

// Hardware pin definitions for Pycom LoPy4 board
#define PIN_SPI_SS    GPIO_NUM_18
#define PIN_SPI_MOSI  GPIO_NUM_27
#define PIN_SPI_MISO  GPIO_NUM_19
#define PIN_SPI_SCK   GPIO_NUM_5
#define RST   LMIC_UNUSED_PIN
#define DIO0  GPIO_NUM_23 // LoRa IRQ
#define DIO1  GPIO_NUM_23 // workaround
#define DIO2  LMIC_UNUSED_PIN

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  21      // pin for switching wifi antenna
#define WIFI_ANTENNA 0              // 0 = internal, 1 = external