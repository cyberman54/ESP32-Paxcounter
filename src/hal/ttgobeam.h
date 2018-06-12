// Hardware related definitions for TTGO T-Beam board

#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_LED GPIO_NUM_21 // on board green LED_G1
//#define HAS_BUTTON GPIO_NUM_39 // on board button "BOOT" (next to reset
// button) !! seems not to work!!
#define HAS_BATTERY_PROBE                                                      \
  ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board
#define HAS_GPS 1     // use on board GPS
#define GPS_SERIAL                                                             \
  9600, SERIAL_8N1, GPIO_NUM_12,                                               \
      GPIO_NUM_15 // UBlox NEO 6M or 7M with default configuration

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS                                                             \
  GPIO_NUM_18 // ESP32 GPIO18 (Pin18) -- HPD13A NSS/SEL (Pin4) SPI Chip Select
              // Input
#define PIN_SPI_MOSI                                                           \
  GPIO_NUM_27 // ESP32 GPIO27 (Pin27) -- HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO                                                           \
  GPIO_NUM_19 // ESP32 GPIO19 (Pin19) -- HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK                                                            \
  GPIO_NUM_5 // ESP32 GPIO5 (Pin5)   -- HPD13A SCK (Pin5) SPI Clock Input

// non arduino pin definitions
#define RST LMIC_UNUSED_PIN // connected to ESP32 RST/EN
#define DIO0 GPIO_NUM_26    // ESP32 GPIO26 <-> HPD13A IO0
#define DIO1 GPIO_NUM_32 // Lora1 <-> HPD13A IO1 // !! NEEDS EXTERNAL WIRING !!
#define DIO2                                                                   \
  LMIC_UNUSED_PIN // Lora2 <-> HPD13A IO2 // needs external wiring, but not
                  // necessary for LoRa, only FSK
