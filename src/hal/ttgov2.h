// Hardware related definitions for TTGO V2 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED NOT_A_PIN // on-board LED is wired to SCL (used by display) therefore totally useless

// disable brownout detection (needed on TTGOv2 for battery powered operation)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    GPIO_NUM_18 // ESP32 GPIO18 (Pin18) -- HPD13A NSS/SEL (Pin4) SPI Chip Select Input
#define PIN_SPI_MOSI  GPIO_NUM_27 // ESP32 GPIO27 (Pin27) -- HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO  GPIO_NUM_19 // ESP32 GPIO19 (Pin19) -- HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK   GPIO_NUM_5  // ESP32 GPIO5 (Pin5)   -- HPD13A SCK (Pin5) SPI Clock Input

// non arduino pin definitions
#define RST   LMIC_UNUSED_PIN // connected to ESP32 RST/EN
#define DIO0  GPIO_NUM_26 // ESP32 GPIO26 wired on PCB to HPD13A
#define DIO1  GPIO_NUM_33 // HPDIO1 on pcb, needs to be wired external to GPIO33
#define DIO2  LMIC_UNUSED_PIN // 32 HPDIO2 on pcb, needs to be wired external to GPIO32 (not necessary for LoRa, only FSK)

// Hardware pin definitions for TTGO V2 Board with OLED SSD1306 0,96" I2C Display
#define OLED_RST U8X8_PIN_NONE // connected to CPU RST/EN
#define OLED_SDA GPIO_NUM_21  // ESP32 GPIO21 -- SD1306 D1+D2
#define OLED_SCL GPIO_NUM_22 // ESP32 GPIO22 -- SD1306 D0

/* source:
https://www.thethingsnetwork.org/forum/t/big-esp32-sx127x-topic-part-2/11973

TTGO LoRa32 V2:
ESP32          LoRa (SPI)      Display (I2C)  LED
-----------    ----------      -------------  ------------------
GPIO5  SCK     SCK
GPIO27 MOSI    MOSI
GPIO19 MISO    MISO
GPIO18 SS      NSS
EN     RST     RST
GPIO26         DIO0
GPIO33         DIO1 (see #1)
GPIO32         DIO2 (see #2)
GPIO22 SCL                     SCL
GPIO21 SDA                     SDA
GPIO22                                        useless (see #3)

#1 Required (used by LMIC for LoRa).
Not on-board wired to any GPIO. Must be manually wired. <<-- necessary for paxcounter

#2 Optional (used by LMIC for FSK but not for LoRa). <<-- NOT necessary for paxcounter
Not on-board wired to any GPIO. When needed: must be manually wired.

#3 GPIO22 is already used for SCL therefore LED cannot be used without conflicting with I2C and display.
*/ 