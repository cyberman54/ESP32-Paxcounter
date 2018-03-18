// Hardware related definitions for TTGO V2 Board

#define TTGO
#define HAS_DISPLAY // has OLED-Display
#define CFG_sx1276_radio 1

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    18 // ESP32 GPIO18 (Pin18) -- SX1276 NSS (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  27 // ESP32 GPIO27 (Pin27) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   5  // ESP32 GPIO5 (Pin5)   -- SX1276 SCK (Pin16) SPI Clock Input

// non arduino pin definitions
#define RST   LMIC_UNUSED_PIN // not sure
#define DIO0  26 // wired on PCB
#define DIO1  33 // needs to be wired external
#define DIO2  32 // needs to be wired external (but not necessary for LoRa)

// Hardware pin definitions for TTGO V2 Board with OLED SSD1306 0,96" I2C Display
#define OLED_RST U8X8_PIN_NONE // to be checked if really not connected
#define OLED_SDA 21  // ESP32 GPIO4 (Pin4)   -- SD1306 Data
#define OLED_SCL 22 // ESP32 GPIO15 (Pin15) -- SD1306 Clock


/*
 ESP32             LoRa module (SPI)        OLED display (I2C)
   ---------         -----------------        ------------------
    5  SCK           SCK
   27  MOSI          MOSI
   19  MISO          MISO
   18  SS            NSS
   14                RST
   26                DIO0
   33                DIO1  (see note {1})
   32                DIO2  (see note {2})
   22  SCL                                    SCL
   21  SDA                                    SDA
   22  LED  (useless, see note {3})

   {1} Must be manually wired!
       DIO1 is wired to a separate pin but is not wired on-board to pin/GPIO33.
       Explicitly wire board pin labeled DIO1 to pin 33 (see TTGO V2.0 pinout).
   {2} Must be manually wired!
       DIO2 is wired to a separate pin but is not wired on-board to pin/GPIO32.
       Explicitly wire board pin labeled DIO2 to pin 32 (see TTGO V2.0 pinout).
   {3} The on-board LED is wired to SCL (used by display) therefore totally useless!
   */

