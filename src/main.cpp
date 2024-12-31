/*

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

Copyright  2018-2020 Oliver Brandmueller <ob@sysadm.in>
Copyright  2018-2020 Klaus Wilting <verkehrsrot@arcor.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTE:
Parts of the source files in this repository are made available under different
licenses. Refer to LICENSE.txt file in repository for more details.

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

// Tasks and timers:

Task          	Core  Prio  Purpose
-------------------------------------------------------------------------------
ledloop*      	1     1    blinks LEDs
buttonloop*     1     2    reads button
spiloop#      	0     2    reads/writes data on spi interface
lmictask*     	1     2    MCCI LMiC LORAWAN stack
clockloop#    	1     6    generates realtime telegrams for external clock
mqttloop#     	1     5    reads/writes data on ETH interface
timesync_proc#	1     7    processes realtime time sync requests
irqhandler#   	1     4    application IRQ (i.e. displayrefresh)
gpsloop*      	1     1    reads data from GPS via serial or i2c
lorasendtask# 	1     2    feeds data from lora sendqueue to lmcic
rmcd_process# 	1     1    Remote command interpreter loop

* spinning task, always ready
# blocked/waiting task

Low priority numbers denote low priority tasks.
-------------------------------------------------------------------------------

// ESP32 hardware timers
-------------------------------------------------------------------------------
0	displayIRQ -> display refresh -> 40ms (DISPLAYREFRESH_MS)
1 ppsIRQ -> pps clock irq -> 1sec
2 (unused)
3	MatrixDisplayIRQ -> matrix mux cycle -> 0,5ms (MATRIX_DISPLAY_SCAN_US)


// External RTC timer (if present)
-------------------------------------------------------------------------------
triggers pps 1 sec impulse


// Interrupt routines
-------------------------------------------------------------------------------

ISRs fired by CPU or GPIO:
DisplayIRQ      <- esp32 timer 0
CLOCKIRQ        <- esp32 timer 1 or GPIO (RTC_INT)
MatrixDisplayIRQ<- esp32 timer 3
ButtonIRQ       <- GPIO <- Button
PMUIRQ          <- GPIO <- PMU chip

Application IRQs fired by software:
TIMESYNC_IRQ    <- setTimeSyncIRQ() <- Ticker.h
CYCLIC_IRQ      <- setCyclicIRQ() <- Ticker.h
SENDCYCLE_IRQ   <- setSendIRQ() <- libpax callback
BME_IRQ         <- setBMEIRQ() <- Ticker.h

*/

// Basic Config
#include "main.h"
#include "mqtthandler.h"

#ifndef HOMECYCLE
#define HOMECYCLE 30  // Default home cycle in seconds if not defined elsewhere
#endif

#ifndef WIFI_MY_COUNTRY
#define WIFI_MY_COUNTRY "01"  // Default to world safe mode if not defined elsewhere
#endif

#ifndef HAS_LED
#define HAS_LED NOT_A_PIN  // Default to no LED if not defined elsewhere
#endif

char clientId[20] = {0}; // unique ClientID

void setup() {
  char features[100] = "";

  // Reduce power consumption (optional)
  // This reduces the power consumption with about 50 mWatt.
  // Typically a TTGO T-beam v1.0 uses 660 mWatt when the CPU frequency is set to 80 MHz.
  // When left running at 240 mHz, the power consumption is about 710 - 730 mWatt.
  // Higher CPU speed may be preferred for wifi & ble sniffing.
  //
  // setCpuFrequencyMhz(80);

  // disable brownout detection
#ifdef DISABLE_BROWNOUT
  // register with brownout is at address DR_REG_RTCCNTL_BASE + 0xd4
  (*((uint32_t volatile *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;
#endif

  // hash 6 byte device MAC to 4 byte clientID
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  const uint32_t hashedmac = myhash((const char *)mac, 6);
  snprintf(clientId, 20, "paxcounter_%08x", hashedmac);

  // setup debug output or silence device
#if (VERBOSE)
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
#else
  // mute logs completely by redirecting them to silence function
  esp_log_level_set("*", ESP_LOG_NONE);
#endif

// initialize SD interface and mount SD card, if present
#if (HAS_SDCARD)
  if (sdcard_init())
    strcat_P(features, " SD");
#endif

  // load device configuration from NVRAM and set runmode
  do_after_reset();

  ESP_LOGI(TAG, "Starting %s v%s (runmode=%d / restarts=%d)", clientId,
           PROGVERSION, RTC_runmode, RTC_restarts);
  ESP_LOGI(TAG, "code build date: %d", compileTime());

  // print chip information on startup if in verbose mode after coldstart
#if (VERBOSE)

  if (RTC_runmode == RUNMODE_POWERCYCLE) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG,
             "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
             "%d, %dMB %s Flash",
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
             chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                           : "external");
    ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
             ESP.getHeapSize(), ESP.getFreeHeap());
#ifdef BOARD_HAS_PSRAM
    ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d",
             ESP.getPsramSize(), ESP.getFreePsram());
#endif
    ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
             ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
    ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
             ESP.getFlashChipSpeed());
    ESP_LOGI(TAG, "Wifi/BT software coexist version %s",
             esp_coex_version_get());
    ESP_LOGI(TAG, "Wifi STA MAC: %s",
             WiFi.macAddress().c_str());

#if (HAS_LORA)
    ESP_LOGI(TAG, "IBM LMIC version %d.%d.%d", LMIC_VERSION_MAJOR,
             LMIC_VERSION_MINOR, LMIC_VERSION_BUILD);
    ESP_LOGI(TAG, "Arduino LMIC version %d.%d.%d.%d",
             ARDUINO_LMIC_VERSION_GET_MAJOR(ARDUINO_LMIC_VERSION),
             ARDUINO_LMIC_VERSION_GET_MINOR(ARDUINO_LMIC_VERSION),
             ARDUINO_LMIC_VERSION_GET_PATCH(ARDUINO_LMIC_VERSION),
             ARDUINO_LMIC_VERSION_GET_LOCAL(ARDUINO_LMIC_VERSION));
    showLoraKeys();
#endif // HAS_LORA

#if (HAS_GPS)
    ESP_LOGI(TAG, "TinyGPS+ version %s", TinyGPSPlus::libraryVersion());
#endif
  }
#endif // VERBOSE

  // open i2c bus
  i2c_init();

// setup power on boards with power management logic
#ifdef EXT_POWER_SW
  pinMode(EXT_POWER_SW, OUTPUT);
  digitalWrite(EXT_POWER_SW, EXT_POWER_ON);
  strcat_P(features, " VEXT");
#endif

#if defined HAS_PMU || defined HAS_IP5306
#ifdef HAS_PMU
  PMU_init();
#elif defined HAS_IP5306
  IP5306_init();
#endif
  strcat_P(features, " PMU");
#endif

  // now that we are powered, we scan i2c bus for devices
  if (RTC_runmode == RUNMODE_POWERCYCLE)
    i2c_scan();

// initialize display
#ifdef HAS_DISPLAY
  strcat_P(features, " DISP");
  DisplayIsOn = cfg.screenon;
  // display verbose info only after a coldstart (note: blocking call!)
  dp_init(RTC_runmode == RUNMODE_POWERCYCLE ? true : false);
#endif

#ifdef BOARD_HAS_PSRAM
  _ASSERT(psramFound());
  ESP_LOGI(TAG, "PSRAM found and initialized");
  strcat_P(features, " PSRAM");
#endif

#ifdef BAT_MEASURE_EN
  pinMode(BAT_MEASURE_EN, OUTPUT);
#endif

// initialize leds
#ifdef HAS_RGB_LED
  rgb_led_init();
  strcat_P(features, " RGB");
#endif

#if (HAS_LED != NOT_A_PIN)
  pinMode(HAS_LED, OUTPUT);
  strcat_P(features, " LED");
#ifdef LED_POWER_SW
  pinMode(LED_POWER_SW, OUTPUT);
  digitalWrite(LED_POWER_SW, LED_POWER_ON);
#endif
#ifdef HAS_TWO_LED
  pinMode(HAS_TWO_LED, OUTPUT);
  strcat_P(features, " LED2");
#endif
// use LED for power display if we have additional RGB LED, else for status
#ifdef HAS_RGB_LED
  switch_LED(LED_ON);
#endif
#endif // HAS_LED

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  // start led loop
  ESP_LOGI(TAG, "Starting LED Controller...");
  xTaskCreatePinnedToCore(ledLoop,      // task function
                          "ledloop",    // name of task
                          1024,         // stack size of task
                          (void *)1,    // parameter of the task
                          1,            // priority of the task
                          &ledLoopTask, // task handle
                          1);           // CPU core
#endif

// initialize wifi antenna
#ifdef HAS_ANTENNA_SWITCH
  strcat_P(features, " ANT");
  antenna_init();
  antenna_select(cfg.wifiant);
#endif

// initialize battery status
#if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
  strcat_P(features, " BATT");
  calibrate_voltage();
  batt_level = read_battlevel();
#ifdef HAS_IP5306
  printIP5306Stats();
#endif
#endif

#if (USE_OTA)
  strcat_P(features, " OTA");
  // reboot to firmware update mode if ota trigger switch is set
  if (RTC_runmode == RUNMODE_UPDATE)
    start_ota_update();
#endif

#if (BOOTMENU)
  // start local webserver after each coldstart
  if (RTC_runmode == RUNMODE_POWERCYCLE)
    start_boot_menu();
#endif

#if defined HAS_IF482
  strcat_P(features, " IF482");
#endif

  // start local webserver on rcommand request
  if (RTC_runmode == RUNMODE_MAINTENANCE)
    start_boot_menu();

  // start libpax lib (includes timer to trigger cyclic senddata)
  ESP_LOGI(TAG, "Starting libpax...");
  struct libpax_config_t configuration;
  libpax_default_config(&configuration);

  // configure WIFI sniffing
  strcpy(configuration.wifi_my_country_str, WIFI_MY_COUNTRY);
  configuration.wificounter = cfg.wifiscan;
  configuration.wifi_channel_map = cfg.wifichanmap;
  configuration.wifi_channel_switch_interval = cfg.wifichancycle;
  configuration.wifi_rssi_threshold = cfg.rssilimit;
  ESP_LOGI(TAG, "WIFISCAN: %s", cfg.wifiscan ? "on" : "off");

  // configure BLE sniffing
  configuration.blecounter = cfg.blescan;
  configuration.blescantime = cfg.blescantime;
  configuration.ble_rssi_threshold = cfg.rssilimit;
  ESP_LOGI(TAG, "BLESCAN: %s", cfg.blescan ? "on" : "off");

  int config_update = libpax_update_config(&configuration);
  if (config_update != 0) {
    ESP_LOGE(TAG, "Error in libpax configuration.");
  } else {
    init_libpax();
  }

  // start rcommand processing task
  ESP_LOGI(TAG, "Starting rcommand interpreter...");
  rcmd_init();

  // cyclic function interrupts
  ESP_LOGI(TAG, "Attaching cyclic timer...");
  cyclicTimer.attach(HOMECYCLE, setCyclicIRQ);
  ESP_LOGI(TAG, "Cyclic timer attached");

  // show compiled features
  ESP_LOGI(TAG, "Features:%s", features);

  // set runmode to normal
  RTC_runmode = RUNMODE_NORMAL;

  // start state machine
  ESP_LOGI(TAG, "Starting Interrupt Handler...");
  xTaskCreatePinnedToCore(irqHandler,      // task function
                          "irqhandler",    // name of task
                          4096,            // stack size of task
                          (void *)1,       // parameter of the task
                          4,               // priority of the task
                          &irqHandlerTask, // task handle
                          1);              // CPU core

  // starting timers and interrupts
  _ASSERT(irqHandlerTask != NULL); // has interrupt handler task started?
  ESP_LOGI(TAG, "Starting Timers...");

  // Initialize MQTT handler
  pax_mqtt_init();

  vTaskDelete(NULL);
} // setup()

void loop() {
  // Handle MQTT operations
  pax_mqtt_loop();
  
  vTaskDelete(NULL);
}
