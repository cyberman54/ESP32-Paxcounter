/*
ESP32-Paxcounter

Copyright  2018 Oliver Brandmueller <ob@sysadm.in>
Copyright  2018 Klaus Wilting <verkehrsrot@arcor.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE:
Parts of the source files in this repository are made available under different
licenses. Refer to LICENSE.txt file in repository for more details.

*/

// Basic Config
#include "globals.h"

// Does nothing and avoid any compilation error with I2C
#include <Wire.h>

#ifdef HAS_LORA
// LMIC-Arduino LoRaWAN Stack
#include "loraconf.h"
#include <hal/hal.h>
#include <lmic.h>
#endif

// ESP32 lib Functions
#include <esp32-hal-log.h>  // needed for ESP_LOGx on arduino framework
#include <esp_event_loop.h> // needed for Wifi event handler
#include <esp_spi_flash.h>  // needed for reading ESP32 chip attributes

// Initialize global variables
configData_t cfg; // struct holds current device configuration
char display_line6[16], display_line7[16]; // display buffers
uint64_t uptimecounter = 0;                // timer global for uptime counter
uint8_t DisplayState = 0;                  // globals for state machine
uint16_t macs_total = 0, macs_wifi = 0,
         macs_ble = 0; // MAC counters globals for display
uint8_t channel = 0;   // wifi channel rotation counter global for display
led_states LEDState = LED_OFF; // LED state global for state machine
led_states previousLEDState =
    LED_ON; // This will force LED to be off at boot since State is OFF
unsigned long LEDBlinkStarted = 0; // When (in millis() led blink started)
uint16_t LEDBlinkDuration = 0;     // How long the blink need to be
uint16_t LEDColor = COLOR_NONE; // state machine variable to set RGB LED color
hw_timer_t *displaytimer =
    NULL; // configure hardware timer used for cyclic display refresh
hw_timer_t *channelSwitch =
    NULL; // configure hardware timer used for wifi channel switching

#ifdef HAS_GPS
gpsStatus_t gps_status; // struct for storing gps data
TinyGPSPlus gps;        // create TinyGPS++ instance
#endif

portMUX_TYPE timerMux =
    portMUX_INITIALIZER_UNLOCKED; // sync main loop and ISR when modifying IRQ
                                  // handler shared variables

std::set<uint16_t> macs; // associative container holds total of unique MAC
                         // adress hashes (Wifi + BLE)

// initialize payload encoder
#if PAYLOAD_ENCODER == 1
TTNplain payload(PAYLOAD_BUFFER_SIZE);
#elif PAYLOAD_ENCODER == 2
TTNpacked payload(PAYLOAD_BUFFER_SIZE);
#elif PAYLOAD_ENCODER == 3
CayenneLPP payload(PAYLOAD_BUFFER_SIZE);
#else
#error "No valid payload converter defined"
#endif

// this variables will be changed in the ISR, and read in main loop
static volatile int ButtonPressedIRQ = 0, DisplayTimerIRQ = 0,
                    ChannelTimerIRQ = 0;

// local Tag for logging
static const char TAG[] = "main";

#ifndef VERBOSE
int redirect_log(const char *fmt, va_list args) {
  // do nothing
  return 0;
}
#endif

void reset_counters() {
  macs.clear();   // clear all macs container
  macs_total = 0; // reset all counters
  macs_wifi = 0;
  macs_ble = 0;
}

/* begin LMIC specific parts
 * ------------------------------------------------------------ */

#ifdef HAS_LORA

#ifdef VERBOSE
void printKeys(void);
#endif // VERBOSE

// LMIC callback functions
void os_getDevKey(u1_t *buf) { memcpy(buf, APPKEY, 16); }

void os_getArtEui(u1_t *buf) {
  memcpy(buf, APPEUI, 8);
  RevBytes(buf, 8); // TTN requires it in LSB First order, so we swap bytes
}

void os_getDevEui(u1_t *buf) {
  int i = 0, k = 0;
  memcpy(buf, DEVEUI, 8); // get fixed DEVEUI from loraconf.h
  for (i = 0; i < 8; i++) {
    k += buf[i];
  }
  if (k) {
    RevBytes(buf, 8); // use fixed DEVEUI and swap bytes to LSB format
  } else {
    gen_lora_deveui(buf); // generate DEVEUI from device's MAC
  }

// Get MCP 24AA02E64 hardware DEVEUI (override default settings if found)
#ifdef MCP_24AA02E64_I2C_ADDRESS
  get_hard_deveui(buf);
  RevBytes(buf, 8); // swap bytes to LSB format
#endif
}

// LMIC enhanced Pin mapping
const lmic_pinmap lmic_pins = {.mosi = PIN_SPI_MOSI,
                               .miso = PIN_SPI_MISO,
                               .sck = PIN_SPI_SCK,
                               .nss = PIN_SPI_SS,
                               .rxtx = LMIC_UNUSED_PIN,
                               .rst = RST,
                               .dio = {DIO0, DIO1, DIO2}};

// LMIC FreeRTos Task
void lorawan_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {
    os_runloop_once();                  // execute LMIC jobs
    vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog
  }
}

#endif // HAS_LORA

/* end LMIC specific parts
 * --------------------------------------------------------------- */

/* beginn hardware specific parts
 * -------------------------------------------------------- */

#ifdef HAS_DISPLAY
HAS_DISPLAY u8x8(OLED_RST, OLED_SCL, OLED_SDA);
// Display Refresh IRQ
void IRAM_ATTR DisplayIRQ() {
  portENTER_CRITICAL_ISR(&timerMux);
  DisplayTimerIRQ++;
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif

#ifdef HAS_ANTENNA_SWITCH
// defined in antenna.cpp
void antenna_init();
void antenna_select(const uint8_t _ant);
#endif

#ifndef BLECOUNTER
bool btstop = btStop();
#endif

// Button IRQ Handler Routine, IRAM_ATTR necessary here, see
// https://github.com/espressif/arduino-esp32/issues/855
#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ() { ButtonPressedIRQ++; }
#endif

// Wifi Channel Rotation Timer IRQ Handler Routine
void IRAM_ATTR ChannelSwitchIRQ() {
  portENTER_CRITICAL(&timerMux);
  ChannelTimerIRQ++;
  portEXIT_CRITICAL(&timerMux);
}

/* end hardware specific parts
 * -------------------------------------------------------- */

/* begin wifi specific parts
 * ---------------------------------------------------------- */

// Sniffer Task
void sniffer_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {

    if (ChannelTimerIRQ) {
      portENTER_CRITICAL(&timerMux);
      ChannelTimerIRQ--;
      portEXIT_CRITICAL(&timerMux);
      // rotates variable channel 1..WIFI_CHANNEL_MAX
      channel = (channel % WIFI_CHANNEL_MAX) + 1;
      wifi_sniffer_set_channel(channel);
      ESP_LOGD(TAG, "Wifi set channel %d", channel);

      vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog
    }

  } // end of infinite wifi channel rotation loop
}

/* end wifi specific parts
 * ------------------------------------------------------------ */

// uptime counter 64bit to prevent millis() rollover after 49 days
uint64_t uptime() {
  static uint32_t low32, high32;
  uint32_t new_low32 = millis();
  if (new_low32 < low32)
    high32++;
  low32 = new_low32;
  return (uint64_t)high32 << 32 | low32;
}

#ifdef HAS_DISPLAY

#ifdef HAS_LORA
// Print a key on display
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb) {
  const uint8_t *p;
  for (uint8_t i = 0; i < len; i++) {
    p = lsb ? key + len - i - 1 : key + i;
    u8x8.printf("%02X", *p);
  }
  u8x8.printf("\n");
}
#endif // HAS_LORA

void init_display(const char *Productname, const char *Version) {
  uint8_t buf[32];
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
  u8x8.setFlipMode(0);
  u8x8.setInverseFont(1);
  u8x8.draw2x2String(0, 0, Productname);
  u8x8.setInverseFont(0);
  u8x8.draw2x2String(2, 2, Productname);
  delay(1500);
  u8x8.clear();
  u8x8.setFlipMode(1);
  u8x8.setInverseFont(1);
  u8x8.draw2x2String(0, 0, Productname);
  u8x8.setInverseFont(0);
  u8x8.draw2x2String(2, 2, Productname);
  delay(1500);

  u8x8.setFlipMode(0);
  u8x8.clear();

#ifdef DISPLAY_FLIP
  u8x8.setFlipMode(1);
#endif

// Display chip information
#ifdef VERBOSE
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  u8x8.printf("ESP32 %d cores\nWiFi%s%s\n", chip_info.cores,
              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  u8x8.printf("ESP Rev.%d\n", chip_info.revision);
  u8x8.printf("%dMB %s Flash\n", spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
#endif // VERBOSE

  u8x8.print(Productname);
  u8x8.print(" v");
  u8x8.println(PROGVERSION);

#ifdef HAS_LORA
  u8x8.println("DEVEUI:");
  os_getDevEui((u1_t *)buf);
  DisplayKey(buf, 8, true);
#endif // HAS_LORA

  delay(5000);
  u8x8.clear();
}

void refreshDisplay() {
  // update counter (lines 0-1)
  char buff[16];
  snprintf(
      buff, sizeof(buff), "PAX:%-4d",
      (int)macs.size()); // convert 16-bit MAC counter to decimal counter value
  u8x8.draw2x2String(0, 0,
                     buff); // display number on unique macs total Wifi + BLE

  // update GPS status (line 2)
#ifdef HAS_GPS
  u8x8.setCursor(7, 2);
  if (!gps.location.isValid()) // if no fix then display Sats value inverse
  {
    u8x8.setInverseFont(1);
    u8x8.printf("Sats: %.3d", gps.satellites.value());
    u8x8.setInverseFont(0);
  } else
    u8x8.printf("Sats: %.3d", gps.satellites.value());
#endif

    // update bluetooth counter + LoRa SF (line 3)
#ifdef BLECOUNTER
  u8x8.setCursor(0, 3);
  if (cfg.blescan)
    u8x8.printf("BLTH:%-4d", macs_ble);
  else
    u8x8.printf("%s", "BLTH:off");
#endif

#ifdef HAS_LORA
  u8x8.setCursor(11, 3);
  u8x8.printf("SF:");
  if (cfg.adrmode) // if ADR=on then display SF value inverse
    u8x8.setInverseFont(1);
  u8x8.printf("%c%c", lora_datarate[LMIC.datarate * 2],
              lora_datarate[LMIC.datarate * 2 + 1]);
  if (cfg.adrmode) // switch off inverse if it was turned on
    u8x8.setInverseFont(0);
#endif // HAS_LORA

  // update wifi counter + channel display (line 4)
  u8x8.setCursor(0, 4);
  u8x8.printf("WIFI:%-4d", macs_wifi);
  u8x8.setCursor(11, 4);
  u8x8.printf("ch:%02d", channel);

  // update RSSI limiter status & free memory display (line 5)
  u8x8.setCursor(0, 5);
  u8x8.printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d", cfg.rssilimit);
  u8x8.setCursor(10, 5);
  u8x8.printf("%4dKB", ESP.getFreeHeap() / 1024);

#ifdef HAS_LORA
  // update LoRa status display (line 6)
  u8x8.setCursor(0, 6);
  u8x8.printf("%-16s", display_line6);

  // update LMiC event display (line 7)
  u8x8.setCursor(0, 7);
  u8x8.printf("%-16s", display_line7);
#endif // HAS_LORA
}

void updateDisplay() {
  // refresh display according to refresh cycle setting
  if (DisplayTimerIRQ) {
    portENTER_CRITICAL(&timerMux);
    DisplayTimerIRQ--;
    portEXIT_CRITICAL(&timerMux);

    refreshDisplay();

    // set display on/off according to current device configuration
    if (DisplayState != cfg.screenon) {
      DisplayState = cfg.screenon;
      u8x8.setPowerSave(!cfg.screenon);
    }
  }
} // updateDisplay()
#endif // HAS_DISPLAY

#ifdef HAS_BUTTON
void readButton() {
  if (ButtonPressedIRQ) {
    portENTER_CRITICAL(&timerMux);
    ButtonPressedIRQ--;
    portEXIT_CRITICAL(&timerMux);
    ESP_LOGI(TAG, "Button pressed");
    ESP_LOGI(TAG, "Button pressed, resetting device to factory defaults");
    eraseConfig();
    esp_restart();
  }
}
#endif

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)

void blink_LED(uint16_t set_color, uint16_t set_blinkduration) {
  LEDColor = set_color;                 // set color for RGB LED
  LEDBlinkDuration = set_blinkduration; // duration
  LEDBlinkStarted = millis();           // Time Start here
  LEDState = LED_ON;                    // Let main set LED on
}

void led_loop() {
  // Custom blink running always have priority other LoRaWAN led management
  if (LEDBlinkStarted && LEDBlinkDuration) {

    // ESP_LOGI(TAG, "Start=%ld for %g",LEDBlinkStarted, LEDBlinkDuration );

    // Custom blink is finished, let this order, avoid millis() overflow
    if ((millis() - LEDBlinkStarted) >= LEDBlinkDuration) {
      // Led becomes off, and stop blink
      LEDState = LED_OFF;
      LEDBlinkStarted = 0;
      LEDBlinkDuration = 0;
      LEDColor = COLOR_NONE;
    } else {
      // In case of LoRaWAN led management blinked off
      LEDState = LED_ON;
    }

    // No custom blink, check LoRaWAN state
  } else {

#ifdef HAS_LORA
    // LED indicators for viusalizing LoRaWAN state
    if (LMIC.opmode & (OP_JOINING | OP_REJOIN)) {
      LEDColor = COLOR_YELLOW;
      // quick blink 20ms on each 1/5 second
      LEDState = ((millis() % 200) < 20) ? LED_ON : LED_OFF; // TX data pending
    } else if (LMIC.opmode & (OP_TXDATA | OP_TXRXPEND)) {
      LEDColor = COLOR_BLUE;
      // small blink 10ms on each 1/2sec (not when joining)
      LEDState = ((millis() % 500) < 10) ? LED_ON : LED_OFF;
      // This should not happen so indicate a problem
    } else if (LMIC.opmode &
               ((OP_TXDATA | OP_TXRXPEND | OP_JOINING | OP_REJOIN) == 0)) {
      LEDColor = COLOR_RED;
      // heartbeat long blink 200ms on each 2 seconds
      LEDState = ((millis() % 2000) < 200) ? LED_ON : LED_OFF;
    } else {
#endif // HAS_LORA

      // led off
      LEDColor = COLOR_NONE;
      LEDState = LED_OFF;
    }
  }

  // ESP_LOGI(TAG, "state=%d previous=%d Color=%d",LEDState, previousLEDState,
  // LEDColor );
  // led need to change state? avoid digitalWrite() for nothing
  if (LEDState != previousLEDState) {
    if (LEDState == LED_ON) {
      rgb_set_color(LEDColor);

#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_LED, LOW);
#else
    digitalWrite(HAS_LED, HIGH);
#endif

    } else {
      rgb_set_color(COLOR_NONE);

#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_LED, HIGH);
#else
    digitalWrite(HAS_LED, LOW);
#endif
    }
    previousLEDState = LEDState;
  }
}; // led_loop()

#endif // #if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)

void sendpayload() {
  // append counter data to payload
  payload.reset();
  payload.addCount(macs_wifi, cfg.blescan ? macs_ble : 0);
  // append GPS data, if present
#ifdef HAS_GPS
  if ((cfg.gpsmode) && (gps.location.isValid())) {
    gps_read();
    payload.addGPS(gps_status);
  }
#endif
  senddata(PAYLOADPORT);
}

/* begin Aruino SETUP
 * ------------------------------------------------------------ */

void setup() {

  char features[64] = "";

  // disable brownout detection
#ifdef DISABLE_BROWNOUT
  // register with brownout is at address DR_REG_RTCCNTL_BASE + 0xd4
  (*((volatile uint32_t *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;
#endif

  // setup debug output or silence device
#ifdef VERBOSE
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
#else
  // mute logs completely by redirecting them to silence function
  esp_log_level_set("*", ESP_LOG_NONE);
  esp_log_set_vprintf(redirect_log);
#endif

  ESP_LOGI(TAG, "Starting %s v%s", PROGNAME, PROGVERSION);

  // initialize system event handler for wifi task, needed for
  // wifi_sniffer_init()
  esp_event_loop_init(NULL, NULL);

  // print chip information on startup if in verbose mode
#ifdef VERBOSE
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG,
           "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
           "%d, %dMB %s Flash",
           chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                         : "external");
  ESP_LOGI(TAG, "ESP32 SDK: %s", ESP.getSdkVersion());
  ESP_LOGI(TAG, "Free RAM: %d bytes", ESP.getFreeHeap());

#ifdef HAS_GPS
  ESP_LOGI(TAG, "TinyGPS+ v%s", TinyGPSPlus::libraryVersion());
#endif

#endif // verbose

  // read settings from NVRAM
  loadConfig(); // includes initialize if necessary

  // initialize led if needed
#if (HAS_LED != NOT_A_PIN)
  pinMode(HAS_LED, OUTPUT);
  strcat_P(features, " LED");
#endif

#ifdef HAS_RGB_LED
  rgb_set_color(COLOR_PINK);
  strcat_P(features, " RGB");
#endif

  // initialize button handling if needed
#ifdef HAS_BUTTON
  strcat_P(features, " BTN_");
#ifdef BUTTON_PULLUP
  strcat_P(features, "PU");
  // install button interrupt (pullup mode)
  pinMode(HAS_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, RISING);
#else
  strcat_P(features, "PD");
  // install button interrupt (pulldown mode)
  pinMode(HAS_BUTTON, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, FALLING);
#endif // BUTTON_PULLUP
#endif // HAS_BUTTON

  // initialize wifi antenna if needed
#ifdef HAS_ANTENNA_SWITCH
  strcat_P(features, " ANT");
  antenna_init();
#endif

// initialize gps if present
#ifdef HAS_GPS
  strcat_P(features, " GPS");
#endif

#ifdef HAS_DISPLAY
  strcat_P(features, " OLED");
  // initialize display
  init_display(PROGNAME, PROGVERSION);
  DisplayState = cfg.screenon;
  u8x8.setPowerSave(!cfg.screenon); // set display off if disabled
  u8x8.draw2x2String(0, 0, "PAX:0");
#ifdef BLECOUNTER
  u8x8.setCursor(0, 3);
  u8x8.printf("BLTH:0");
#endif
  u8x8.setCursor(0, 4);
  u8x8.printf("WIFI:0");
  u8x8.setCursor(0, 5);
  u8x8.printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%d", cfg.rssilimit);

#ifdef HAS_LORA
  sprintf(display_line6, "Join wait");
#endif // HAS_LORA

  // setup display refresh trigger IRQ using esp32 hardware timer 0
  // for explanation see
  // https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
  displaytimer = timerBegin(0, 80, true); // prescaler 80 -> divides 80 MHz CPU
                                          // freq to 1 MHz, timer 0, count up
  timerAttachInterrupt(displaytimer, &DisplayIRQ,
                       true); // interrupt handler DisplayIRQ, triggered by edge
  timerAlarmWrite(
      displaytimer, DISPLAYREFRESH_MS * 1000,
      true); // reload interrupt after each trigger of display refresh cycle
  timerAlarmEnable(displaytimer); // enable display interrupt
#endif

  // setup channel rotation trigger IRQ using esp32 hardware timer 1
  channelSwitch = timerBegin(1, 80, true);
  timerAttachInterrupt(channelSwitch, &ChannelSwitchIRQ, true);
  timerAlarmWrite(channelSwitch, cfg.wifichancycle * 10000, true);
  timerAlarmEnable(channelSwitch);

// show payload encoder
#if PAYLOAD_ENCODER == 1
  strcat_P(features, " PAYLOAD_PLAIN");
#elif PAYLOAD_ENCODER == 2
  strcat_P(features, " PAYLOAD_PACKED");
#elif PAYLOAD_ENCODER == 3
  strcat_P(features, " PAYLOAD_CAYENNE");
#endif

  // show compiled features
  ESP_LOGI(TAG, "Features: %s", features);

#ifdef HAS_LORA

  // output LoRaWAN keys to console
#ifdef VERBOSE
  printKeys();
#endif

  // initialize LoRaWAN LMIC run-time environment
  os_init();
  // reset LMIC MAC state
  LMIC_reset();
  // This tells LMIC to make the receive windows bigger, in case your clock is
  // 1% faster or slower.
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // start lmic runloop in rtos task on core 1 (note: arduino main loop runs on
  // core 1, too)
  // https://techtutorialsx.com/2017/05/09/esp32-get-task-execution-core/

  ESP_LOGI(TAG, "Starting Lora task on core 1");
  xTaskCreatePinnedToCore(lorawan_loop, "loratask", 2048, (void *)1,
                          (5 | portPRIVILEGE_BIT), NULL, 1);
#endif

  // start wifi in monitor mode and start channel rotation task on core 0
  ESP_LOGI(TAG, "Starting Wifi task on core 0");
  wifi_sniffer_init();
  // initialize salt value using esp_random() called by random() in
  // arduino-esp32 core note: do this *after* wifi has started, since function
  // gets it's seed from RF noise
  reset_salt(); // get new 16bit for salting hashes
  xTaskCreatePinnedToCore(sniffer_loop, "wifisniffer", 2048, (void *)1, 1, NULL,
                          0);

// start BLE scan callback if BLE function is enabled in NVRAM configuration
#ifdef BLECOUNTER
  if (cfg.blescan) {
    start_BLEscan();
  }
#endif

// if device has GPS and it is enabled, start GPS reader task on core 0
// higher priority than wifi channel rotation task since we process serial
// streaming NMEA data
#ifdef HAS_GPS
  if (cfg.gpsmode) {
    ESP_LOGI(TAG, "Starting GPS task on core 0");
    xTaskCreatePinnedToCore(gps_loop, "gpsfeed", 2048, (void *)1, 2, NULL, 0);
  }
#endif

  // send initial payload to open transfer interfaces
  sendpayload();
}

/* end Arduino SETUP
 * ------------------------------------------------------------ */

/* begin Arduino main loop
 * ------------------------------------------------------ */

void loop() {

  while (1) {

    // simple state machine for controlling uptime, display, LED, button,
    // memory.

    uptimecounter = uptime() / 1000; // counts uptime in seconds (64bit)

    // send data every x seconds, x/2 is configured in cfg.sendcycle
    if ((uptime() % (cfg.sendcycle * 2000)) < 1)
      sendpayload();

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
    led_loop();
#endif

#ifdef HAS_BUTTON
    readButton();
#endif

#ifdef HAS_DISPLAY
    updateDisplay();
#endif

    // check free memory
    if (esp_get_minimum_free_heap_size() <= MEM_LOW) {
      ESP_LOGI(TAG,
               "Memory full, counter cleared (heap low water mark = %d Bytes / "
               "free heap = %d bytes)",
               esp_get_minimum_free_heap_size(), ESP.getFreeHeap());
      senddata(PAYLOADPORT); // send data before clearing counters
      reset_counters();      // clear macs container and reset all counters
      reset_salt();          // get new salt for salting hashes
    }

#ifdef HAS_GPS
    // log NMEA status every 60 seconds, useful for debugging GPS connection
    if ((uptime() % 60000) < 1) {
      ESP_LOGD(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
               gps.passedChecksum(), gps.failedChecksum(),
               gps.sentencesWithFix());
      if ((cfg.gpsmode) && (gps.location.isValid())) {
        gps_read();
        ESP_LOGI(TAG,
                 "lat=%.6f | lon=%.6f | %u Sats | HDOP=%.1f | Altitude=%um",
                 gps_status.latitude / (float)1e6,
                 gps_status.longitude / (float)1e6, gps_status.satellites,
                 gps_status.hdop / (float)100, gps_status.altitude);
      } else {
        ESP_LOGI(TAG, "No valid GPS position or GPS disabled");
      }
    }
#endif

    vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog

  } // end of infinite main loop
}

/* end Arduino main loop
 * ------------------------------------------------------------ */
