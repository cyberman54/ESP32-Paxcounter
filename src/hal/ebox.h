// Hardware related definitions for ebox ESP32-bit with external connected RFM95 LoRa

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1

#define HAS_LED GPIO_NUM_23 // blue LED on board
#define HAS_BUTTON GPIO_NUM_0 // button "PROG" on board

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    GPIO_NUM_18 // ESP32 GPIO18 (Pin18) -- SX1276 NSS (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  GPIO_NUM_27 // ESP32 GPIO27 (Pin27) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  GPIO_NUM_19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   GPIO_NUM_5  // ESP32 GPIO5 (Pin5)   -- SX1276 SCK (Pin16) SPI Clock Input

// non arduino pin definitions
#define RST   GPIO_NUM_14 // ESP32 GPIO14 (Pin14) -- SX1276 NRESET (Pin7) Reset Trigger Input
#define DIO0  GPIO_NUM_26 // ESP32 GPIO26 (Pin15) -- SX1276 DIO0 (Pin8) used by LMIC for detecting LoRa RX_Done & TX_Done
#define DIO1  GPIO_NUM_33 // ESP32 GPIO33 (Pin13) -- SX1276 DIO1 (Pin9) used by LMIC for detecting LoRa RX_Timeout
#define DIO2  LMIC_UNUSED_PIN // 32 ESP32 GPIO32 (Pin12) -- SX1276 DIO2 (Pin10) not used by LMIC for LoRa (Timeout for FSK only)