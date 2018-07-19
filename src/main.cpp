/*

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

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
#include "main.h"

configData_t cfg; // struct holds current device configuration
char display_line6[16], display_line7[16]; // display buffers
uint8_t channel = 0;                       // channel rotation counter
uint16_t macs_total = 0, macs_wifi = 0,
         macs_ble = 0; // MAC counters globals for display
hw_timer_t *channelSwitch = NULL, *displaytimer = NULL,
           *sendCycle = NULL; // configure hardware timer for cyclic tasks

// this variables will be changed in the ISR, and read in main loop
static volatile int ButtonPressedIRQ = 0, ChannelTimerIRQ = 0,
                    SendCycleTimerIRQ = 0, DisplayTimerIRQ = 0;

portMUX_TYPE timerMux =
    portMUX_INITIALIZER_UNLOCKED; // sync main loop and ISR when modifying IRQ
                                  // handler shared variables

std::set<uint16_t> macs; // associative container holds total of unique MAC
                         // adress hashes (Wifi + BLE)

// initialize payload ncoder
PayloadConvert payload(PAYLOAD_BUFFER_SIZE);

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

#ifdef HAS_LORA

// LMIC enhanced Pin mapping
const lmic_pinmap lmic_pins = {.mosi = PIN_SPI_MOSI,
                               .miso = PIN_SPI_MISO,
                               .sck = PIN_SPI_SCK,
                               .nss = PIN_SPI_SS,
                               .rxtx = LMIC_UNUSED_PIN,
                               .rst = RST,
                               .dio = {DIO0, DIO1, DIO2}};

// Get MCP 24AA02E64 hardware DEVEUI (override default settings if found)
#ifdef MCP_24AA02E64_I2C_ADDRESS
get_hard_deveui(buf);
RevBytes(buf, 8); // swap bytes to LSB format
#endif

// LMIC FreeRTos Task
void lorawan_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {
    os_runloop_once();                  // execute LMIC jobs
    vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog
  }
}

#endif // HAS_LORA

// Setup IRQ handler routines
// attn see https://github.com/espressif/arduino-esp32/issues/855

void IRAM_ATTR ChannelSwitchIRQ() {
  portENTER_CRITICAL(&timerMux);
  ChannelTimerIRQ++;
  portEXIT_CRITICAL(&timerMux);
}

void IRAM_ATTR SendCycleIRQ() {
  portENTER_CRITICAL(&timerMux);
  SendCycleTimerIRQ++;
  portEXIT_CRITICAL(&timerMux);
}

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ() {
  portENTER_CRITICAL_ISR(&timerMux);
  DisplayTimerIRQ++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void updateDisplay() {
  // refresh display according to refresh cycle setting
  if (DisplayTimerIRQ) {
    portENTER_CRITICAL(&timerMux);
    DisplayTimerIRQ = 0;
    portEXIT_CRITICAL(&timerMux);
    refreshDisplay();
  }
}

#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ() { ButtonPressedIRQ++; }

void readButton() {
  if (ButtonPressedIRQ) {
    portENTER_CRITICAL(&timerMux);
    ButtonPressedIRQ = 0;
    portEXIT_CRITICAL(&timerMux);
    ESP_LOGI(TAG, "Button pressed");
    ESP_LOGI(TAG, "Button pressed, resetting device to factory defaults");
    eraseConfig();
    esp_restart();
  }
}
#endif

// Wifi channel rotation task
void wifi_channel_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {

    if (ChannelTimerIRQ) {
      portENTER_CRITICAL(&timerMux);
      ChannelTimerIRQ = 0;
      portEXIT_CRITICAL(&timerMux);
      // rotates variable channel 1..WIFI_CHANNEL_MAX
      channel = (channel % WIFI_CHANNEL_MAX) + 1;
      wifi_sniffer_set_channel(channel);
      ESP_LOGD(TAG, "Wifi set channel %d", channel);

      vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog
    }

  } // end of infinite wifi channel rotation loop
}

// uptime counter 64bit to prevent millis() rollover after 49 days
uint64_t uptime() {
  static uint32_t low32, high32;
  uint32_t new_low32 = millis();
  if (new_low32 < low32)
    high32++;
  low32 = new_low32;
  return (uint64_t)high32 << 32 | low32;
}

void sendPayload() {

  if (SendCycleTimerIRQ) {
    portENTER_CRITICAL(&timerMux);
    SendCycleTimerIRQ = 0;
    portEXIT_CRITICAL(&timerMux);

    // append counter data to payload
    payload.reset();
    payload.addCount(macs_wifi, cfg.blescan ? macs_ble : 0);
    // append GPS data, if present

#ifdef HAS_GPS
    if ((cfg.gpsmode) && (gps.location.isValid())) {
      gps_read();
      payload.addGPS(gps_status);
    }
    // log NMEA status, useful for debugging GPS connection
    ESP_LOGD(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
             gps.passedChecksum(), gps.failedChecksum(),
             gps.sentencesWithFix());
    // log GPS position if we have a fix
    if ((cfg.gpsmode) && (gps.location.isValid())) {
      gps_read();
      ESP_LOGI(TAG, "lat=%.6f | lon=%.6f | %u Sats | HDOP=%.1f | Altitude=%um",
               gps_status.latitude / (float)1e6,
               gps_status.longitude / (float)1e6, gps_status.satellites,
               gps_status.hdop / (float)100, gps_status.altitude);
    } else {
      ESP_LOGI(TAG, "No valid GPS position or GPS disabled");
    }
#endif

    senddata(PAYLOADPORT);
  }
} // sendPayload()

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
  antenna_select(cfg.wifiant);
#endif

// switch off bluetooth on esp32 module, if not compiled
#ifdef BLECOUNTER
  strcat_P(features, " BLE");
#else
  bool btstop = btStop();
#endif

// initialize gps if present
#ifdef HAS_GPS
  strcat_P(features, " GPS");
#endif

// initialize display if present
#ifdef HAS_DISPLAY
  strcat_P(features, " OLED");
  DisplayState = cfg.screenon;
  init_display(PROGNAME, PROGVERSION);

  // setup display refresh trigger IRQ using esp32 hardware timer
  // https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/

  // prescaler 80 -> divides 80 MHz CPU freq to 1 MHz, timer 0, count up
  displaytimer = timerBegin(0, 80, true);
  // interrupt handler DisplayIRQ, triggered by edge
  timerAttachInterrupt(displaytimer, &DisplayIRQ, true);
  // reload interrupt after each trigger of display refresh cycle
  timerAlarmWrite(displaytimer, DISPLAYREFRESH_MS * 1000, true);
  // enable display interrupt
  timerAlarmEnable(displaytimer);
#endif

  // setup channel rotation trigger IRQ using esp32 hardware timer 1
  channelSwitch = timerBegin(1, 800, true);
  timerAttachInterrupt(channelSwitch, &ChannelSwitchIRQ, true);
  timerAlarmWrite(channelSwitch, cfg.wifichancycle * 1000, true);
  timerAlarmEnable(channelSwitch);

  // setup send cycle trigger IRQ using esp32 hardware timer 2
  sendCycle = timerBegin(2, 8000, true);
  timerAttachInterrupt(sendCycle, &SendCycleIRQ, true);
  timerAlarmWrite(sendCycle, cfg.sendcycle * 2 * 10000, true);
  timerAlarmEnable(sendCycle);

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
  // join network
  LMIC_startJoining();

  // start lmic runloop in rtos task on core 1
  // (note: arduino main loop runs on core 1, too)
  // https://techtutorialsx.com/2017/05/09/esp32-get-task-execution-core/

  ESP_LOGI(TAG, "Starting Lora task on core 1");
  xTaskCreatePinnedToCore(lorawan_loop, "loraloop", 2048, (void *)1,
                          (5 | portPRIVILEGE_BIT), NULL, 1);
#endif

// if device has GPS and it is enabled, start GPS reader task on core 0 with
// higher priority than wifi channel rotation task since we process serial
// streaming NMEA data
#ifdef HAS_GPS
  if (cfg.gpsmode) {
    ESP_LOGI(TAG, "Starting GPS task on core 0");
    xTaskCreatePinnedToCore(gps_loop, "gpsloop", 2048, (void *)1, 2, NULL, 0);
  }
#endif

// start BLE scan callback if BLE function is enabled in NVRAM configuration
#ifdef BLECOUNTER
  if (cfg.blescan) {
    ESP_LOGI(TAG, "Starting BLE task on core 1");
    start_BLEscan();
  }
#endif

  // start wifi in monitor mode and start channel rotation task on core 0
  ESP_LOGI(TAG, "Starting Wifi task on core 0");
  wifi_sniffer_init();
  // initialize salt value using esp_random() called by random() in
  // arduino-esp32 core note: do this *after* wifi has started, since function
  // gets it's seed from RF noise
  reset_salt(); // get new 16bit for salting hashes
  xTaskCreatePinnedToCore(wifi_channel_loop, "wifiloop", 2048, (void *)1, 1,
                          NULL, 0);
} // setup

/* end Arduino SETUP
 * ------------------------------------------------------------ */

/* begin Arduino main loop
 * ------------------------------------------------------ */

void loop() {

  while (1) {

    // state machine for uptime, display, LED, button, lowmemory, senddata

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

    // check send cycle and send payload if cycle is expired
    sendPayload();

    vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog

  } // end of infinite main loop
}

/* end Arduino main loop
 * ------------------------------------------------------------ */
