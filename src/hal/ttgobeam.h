// Hardware related definitions for TTGO T-Beam board

#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_LED 21 // on board green LED_G1
#//define HAS_LED 22 // on board red LED_G3 ? to be checked
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7 ? to be checked
#define BATT_FACTOR 2 // voltage divider 100k/100k on board
// #define HAS_GPS // to be done
// GSP serial (9600, SERIAL_8N1, 12, 15);   //17-TX 18-RX

// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    18 // ESP32 GPIO18 (Pin18) -- HPD13A NSS/SEL (Pin4) SPI Chip Select Input
#define PIN_SPI_MOSI  27 // ESP32 GPIO27 (Pin27) -- HPD13A MOSI/DSI (Pin6) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- HPD13A MISO/DSO (Pin7) SPI Data Output
#define PIN_SPI_SCK   5  // ESP32 GPIO5 (Pin5)   -- HPD13A SCK (Pin5) SPI Clock Input

// non arduino pin definitions
#define RST   14 // connected to ESP32 RST/EN
#define DIO0  26 // ESP32 GPIO26 <-> HPD13A IO0
#define DIO1  33 // ESP32 GPIO33 <-> HPDIO1 <-> HPD13A IO1 // !!needs external wiring!!
#define DIO2  32 // ESP32 GPIO32 <-> HPDIO2 <-> HPD13A IO2 // needs external wiring, but not needed for LoRa
