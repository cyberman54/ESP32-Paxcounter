// Hardware related definitions for Pycom LoPy Board (not: LoPy4)

#define CFG_sx1276_radio 1
#define HAS_RGB_LED   0  // WS2812B RGB LED

// Hardware pin definitions for Pycom LoPy4 board
#define PIN_SPI_SS    18
#define PIN_SPI_MOSI  27
#define PIN_SPI_MISO  19
#define PIN_SPI_SCK   5
#define RST   LMIC_UNUSED_PIN
#define DIO0  23 // LoRa IRQ
#define DIO1  23 // workaround
#define DIO2  23 // workaround 

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  21      // pin for switching wifi antenna
#define WIFI_ANTENNA 0              // 0 = internal, 1 = external