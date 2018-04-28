// Hardware related definitions for TTGO V2 Board

#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED NOT_A_PIN // on-board LED is wired to SCL (used by display) therefore totally useless

// disable brownout detection (needed on TTGOv2 for battery powered operation)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    18 // ESP32 GPIO18 (Pin18) -- HPD13A NSS/SEL (Pin4) SPI Chip Select Input
#define PIN_SPI_MOSI  27 // ESP32 GPIO27 (Pin27) -- HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK   5  // ESP32 GPIO5 (Pin5)   -- HPD13A SCK (Pin5) SPI Clock Input

// non arduino pin definitions
#define RST   LMIC_UNUSED_PIN // connected to ESP32 RST/EN
#define DIO0  26 // ESP32 GPIO26 wired on PCB to HPD13A
#define DIO1  33 // HPDIO1 on pcb, needs to be wired external to GPIO33
#define DIO2  LMIC_UNUSED_PIN // 32 HPDIO2 on pcb, needs to be wired external to GPIO32 (not necessary for LoRa, only FSK)

// Hardware pin definitions for TTGO V2 Board with OLED SSD1306 0,96" I2C Display
#define OLED_RST U8X8_PIN_NONE // connected to CPU RST/EN
#define OLED_SDA 21  // ESP32 GPIO4 (Pin4)   -- SD1306 D1+D2
#define OLED_SCL 22 // ESP32 GPIO15 (Pin15) -- SD1306 D0


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

