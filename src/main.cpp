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

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

Uused tasks and timers:

Task          Core  Prio  Purpose
====================================================================================
clockloop     0     4     generates realtime telegrams for external clock
ledloop       0     3     blinks LEDs
spiloop       0     2     reads/writes data on spi interface
IDLE          0     0     ESP32 arduino scheduler -> runs wifi sniffer

looptask      1     1     arduino core -> runs the LMIC LoRa stack
irqhandler    1     1     executes tasks triggered by hw irq, see table below
gpsloop       1     2     reads data from GPS via serial or i2c
bmeloop       1     1     reads data from BME sensor via i2c
IDLE          1     0     ESP32 arduino scheduler -> runs wifi channel rotator

Low priority numbers denote low priority tasks.

Tasks using i2c bus all must have same priority, because using mutex semaphore
(irqhandler, bmeloop)

ESP32 hardware irq timers
================================
 0	triggers display refresh
 1  triggers DCF77 clock signal
 2	triggers send payload cycle
 3	triggers housekeeping cycle

 RTC hardware timer (if present)
================================
 triggers IF482 clock signal

*/

// Basic Config
#include "main.h"

configData_t cfg; // struct holds current device configuration
char display_line6[16], display_line7[16]; // display buffers
uint8_t volatile channel = 0;              // channel rotation counter
uint16_t volatile macs_total = 0, macs_wifi = 0, macs_ble = 0,
                  batt_voltage = 0; // globals for display

hw_timer_t *sendCycle = NULL, *homeCycle = NULL, *clockCycle = NULL,
           *displaytimer = NULL;

TaskHandle_t irqHandlerTask, ClockTask;
SemaphoreHandle_t I2Caccess, TimePulse;
bool volatile TimePulseTick = false;
time_t userUTCTime = 0;

// container holding unique MAC address hashes with Memory Alloctor using PSRAM,
// if present
std::set<uint16_t, std::less<uint16_t>, Mallocator<uint16_t>> macs;

// initialize payload encoder
PayloadConvert payload(PAYLOAD_BUFFER_SIZE);

// set Time Zone for user setting from paxcounter.conf
TimeChangeRule myDST = DAYLIGHT_TIME;
TimeChangeRule mySTD = STANDARD_TIME;
Timezone myTZ(myDST, mySTD);

// local Tag for logging
static const char TAG[] = "main";

void setup() {

  // disable the default wifi logging
  esp_log_level_set("wifi", ESP_LOG_NONE);

  char features[100] = "";

  // create some semaphores for syncing / mutexing tasks
  I2Caccess = xSemaphoreCreateMutex(); // for access management of i2c bus
  if (I2Caccess)
    xSemaphoreGive(I2Caccess); // Flag the i2c bus available for use

  TimePulse = xSemaphoreCreateBinary(); // as signal that shows time pulse flip

  // disable brownout detection
#ifdef DISABLE_BROWNOUT
  // register with brownout is at address DR_REG_RTCCNTL_BASE + 0xd4
  (*((uint32_t volatile *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;
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

  ESP_LOGI(TAG, "Starting %s v%s", PRODUCTNAME, PROGVERSION);

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
  ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
           ESP.getHeapSize(), ESP.getFreeHeap());
#ifdef BOARD_HAS_PSRAM
  ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d", ESP.getPsramSize(),
           ESP.getFreePsram());
#endif
  ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
           ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
           ESP.getFlashChipSpeed());
  ESP_LOGI(TAG, "Wifi/BT software coexist version %s", esp_coex_version_get());

#ifdef HAS_LORA
  ESP_LOGI(TAG, "IBM LMIC version %d.%d.%d", LMIC_VERSION_MAJOR,
           LMIC_VERSION_MINOR, LMIC_VERSION_BUILD);

  ESP_LOGI(TAG, "Arduino LMIC version %d.%d.%d.%d",
           ARDUINO_LMIC_VERSION_GET_MAJOR(ARDUINO_LMIC_VERSION),
           ARDUINO_LMIC_VERSION_GET_MINOR(ARDUINO_LMIC_VERSION),
           ARDUINO_LMIC_VERSION_GET_PATCH(ARDUINO_LMIC_VERSION),
           ARDUINO_LMIC_VERSION_GET_LOCAL(ARDUINO_LMIC_VERSION));
#endif

#ifdef HAS_GPS
  ESP_LOGI(TAG, "TinyGPS+ version %s", TinyGPSPlus::libraryVersion());
#endif

#endif // verbose

  // read (and initialize on first run) runtime settings from NVRAM
  loadConfig(); // includes initialize if necessary

#ifdef BOARD_HAS_PSRAM
  assert(psramFound());
  ESP_LOGI(TAG, "PSRAM found and initialized");
  strcat_P(features, " PSRAM");
#endif

// set low power mode to off
#ifdef HAS_LOWPOWER_SWITCH
  pinMode(HAS_LED, OUTPUT);
  digitalWrite(HAS_LOWPOWER_SWITCH, HIGH);
  strcat_P(features, " LPWR");
#endif

  // initialize leds
#if (HAS_LED != NOT_A_PIN)
  pinMode(HAS_LED, OUTPUT);
  strcat_P(features, " LED");
// switch on power LED if we have 2 LEDs, else use it for status
#ifdef HAS_RGB_LED
  switch_LED(LED_ON);
  strcat_P(features, " RGB");
  rgb_set_color(COLOR_PINK);
#endif
#endif

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  // start led loop
  ESP_LOGI(TAG, "Starting LED Controller...");
  xTaskCreatePinnedToCore(ledLoop,      // task function
                          "ledloop",    // name of task
                          1024,         // stack size of task
                          (void *)1,    // parameter of the task
                          3,            // priority of the task
                          &ledLoopTask, // task handle
                          0);           // CPU core
#endif

// initialize wifi antenna
#ifdef HAS_ANTENNA_SWITCH
  strcat_P(features, " ANT");
  antenna_init();
  antenna_select(cfg.wifiant);
#endif

// initialize battery status
#ifdef HAS_BATTERY_PROBE
  strcat_P(features, " BATT");
  calibrate_voltage();
  batt_voltage = read_voltage();
#endif

#ifdef USE_OTA
  strcat_P(features, " OTA");
  // reboot to firmware update mode if ota trigger switch is set
  if (cfg.runmode == 1) {
    cfg.runmode = 0;
    saveConfig();
    start_ota_update();
  }
#endif

// start BLE scan callback if BLE function is enabled in NVRAM configuration
// or switch off bluetooth, if not compiled
#ifdef BLECOUNTER
  strcat_P(features, " BLE");
  if (cfg.blescan) {
    ESP_LOGI(TAG, "Starting Bluetooth...");
    start_BLEscan();
  } else
    btStop();
#else
  // remove bluetooth stack to gain more free memory
  ESP_ERROR_CHECK(esp_bluedroid_disable());
  ESP_ERROR_CHECK(esp_bluedroid_deinit());
  btStop();
  ESP_ERROR_CHECK(esp_bt_controller_deinit());
  ESP_ERROR_CHECK(esp_bt_mem_release(ESP_BT_MODE_BTDM));
  ESP_ERROR_CHECK(esp_coex_preference_set((
      esp_coex_prefer_t)ESP_COEX_PREFER_WIFI)); // configure Wifi/BT coexist lib
#endif

// initialize button
#ifdef HAS_BUTTON
  strcat_P(features, " BTN_");
#ifdef BUTTON_PULLUP
  strcat_P(features, "PU");
  // install button interrupt (pullup mode)
  pinMode(HAS_BUTTON, INPUT_PULLUP);
#else
  strcat_P(features, "PD");
  // install button interrupt (pulldown mode)
  pinMode(HAS_BUTTON, INPUT_PULLDOWN);
#endif // BUTTON_PULLUP
#endif // HAS_BUTTON

// initialize gps
#ifdef HAS_GPS
  strcat_P(features, " GPS");
  if (gps_init()) {
    ESP_LOGI(TAG, "Starting GPS Feed...");
    xTaskCreatePinnedToCore(gps_loop,  // task function
                            "gpsloop", // name of task
                            2048,      // stack size of task
                            (void *)1, // parameter of the task
                            2,         // priority of the task
                            &GpsTask,  // task handle
                            1);        // CPU core
  }
#endif

// initialize sensors
#ifdef HAS_SENSORS
  strcat_P(features, " SENS");
  sensor_init();
#endif

// initialize LoRa
#ifdef HAS_LORA
  strcat_P(features, " LORA");
#endif
  assert(lora_stack_init() == ESP_OK);

// initialize SPI
#ifdef HAS_SPI
  strcat_P(features, " SPI");
#endif
  assert(spi_init() == ESP_OK);

#ifdef VENDORFILTER
  strcat_P(features, " OUIFLT");
#endif

// initialize display
#ifdef HAS_DISPLAY
  strcat_P(features, " OLED");
  DisplayState = cfg.screenon;
  init_display(PRODUCTNAME, PROGVERSION); // note: blocking call

  // setup display refresh trigger IRQ using esp32 hardware timer
  // https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
  // prescaler 80 -> divides 80 MHz CPU freq to 1 MHz, timer 0, count up
  displaytimer = timerBegin(0, 80, true);
  // interrupt handler DisplayIRQ, triggered by edge
  timerAttachInterrupt(displaytimer, &DisplayIRQ, true);
  // reload interrupt after each trigger of display refresh cycle
  timerAlarmWrite(displaytimer, DISPLAYREFRESH_MS * 1000, true);
#endif

  // setup send cycle trigger IRQ using esp32 hardware timer 2
  sendCycle = timerBegin(2, 8000, true);
  timerAttachInterrupt(sendCycle, &SendCycleIRQ, true);
  timerAlarmWrite(sendCycle, cfg.sendcycle * 2 * 10000, true);

  // setup house keeping cycle trigger IRQ using esp32 hardware timer 3
  homeCycle = timerBegin(3, 8000, true);
  timerAttachInterrupt(homeCycle, &homeCycleIRQ, true);
  timerAlarmWrite(homeCycle, HOMECYCLE * 10000, true);

// show payload encoder
#if PAYLOAD_ENCODER == 1
  strcat_P(features, " PLAIN");
#elif PAYLOAD_ENCODER == 2
  strcat_P(features, " PACKED");
#elif PAYLOAD_ENCODER == 3
  strcat_P(features, " LPPDYN");
#elif PAYLOAD_ENCODER == 4
  strcat_P(features, " LPPPKD");
#endif

  // initialize RTC
#ifdef HAS_RTC
  strcat_P(features, " RTC");
  assert(rtc_init());
#endif

#if defined HAS_DCF77
  strcat_P(features, " DCF77");
#endif

#if defined HAS_IF482
  strcat_P(features, " IF482");
#endif

  // show compiled features
  ESP_LOGI(TAG, "Features:%s", features);

#ifdef HAS_LORA
// output LoRaWAN keys to console
#ifdef VERBOSE
  showLoraKeys();
#endif
#endif

  // start pps timepulse and timekeepr
  ESP_LOGI(TAG, "Starting Timekeeper...");
  assert(timepulse_init()); // setup timepulse
  timepulse_start();
  time_sync();                              // sync time
  setSyncInterval(TIME_SYNC_INTERVAL * 60); // controls timeStatus()

  // start wifi in monitor mode and start channel rotation timer
  ESP_LOGI(TAG, "Starting Wifi...");
  wifi_sniffer_init();
  // initialize salt value using esp_random() called by random() in
  // arduino-esp32 core. Note: do this *after* wifi has started, since
  // function gets it's seed from RF noise
  get_salt(); // get new 16bit for salting hashes

  // start state machine
  ESP_LOGI(TAG, "Starting Interrupt Handler...");
  xTaskCreatePinnedToCore(irqHandler,      // task function
                          "irqhandler",    // name of task
                          4096,            // stack size of task
                          (void *)1,       // parameter of the task
                          1,               // priority of the task
                          &irqHandlerTask, // task handle
                          1);              // CPU core

// initialize bme
#ifdef HAS_BME
  strcat_P(features, " BME");
  if (bme_init()) {
    ESP_LOGI(TAG, "Starting Bluetooth sniffer...");
    xTaskCreatePinnedToCore(bme_loop,  // task function
                            "bmeloop", // name of task
                            2048,      // stack size of task
                            (void *)1, // parameter of the task
                            1,         // priority of the task
                            &BmeTask,  // task handle
                            1);        // CPU core
  }
#endif

  assert(irqHandlerTask != NULL); // has interrupt handler task started?
                                  // start timer triggered interrupts
  ESP_LOGI(TAG, "Starting Interrupts...");
#ifdef HAS_DISPLAY
  timerAlarmEnable(displaytimer);
#endif
  timerAlarmEnable(sendCycle);
  timerAlarmEnable(homeCycle);

// start button interrupt
#ifdef HAS_BUTTON
#ifdef BUTTON_PULLUP
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, RISING);
#else
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, FALLING);
#endif
#endif // HAS_BUTTON

#if defined HAS_IF482 || defined HAS_DCF77
  ESP_LOGI(TAG, "Starting Clock Controller...");
  clock_init();
#endif

} // setup()

void loop() {
  while (1) {
#ifdef HAS_LORA
    os_runloop_once(); // execute lmic scheduled jobs and events
#endif
    delay(2); // yield to CPU
  }

  vTaskDelete(NULL); // shoud never be reached
}