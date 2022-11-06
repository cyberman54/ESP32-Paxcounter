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
SENDCYCLE_IRQ   <- setSendIRQ() <- xTimer or libpax callback
BME_IRQ         <- setBMEIRQ() <- Ticker.h

*/

// Basic Config
#include "main.h"

// local Tag for logging
static const char TAG[] = __FILE__;

char clientId[20] = {0}; // unique ClientID

void setup() {
  char features[100] = "";

  // disable brownout detection
#ifdef DISABLE_BROWNOUT
  // register with brownout is at address DR_REG_RTCCNTL_BASE + 0xd4
  (*((uint32_t volatile *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE + 0xd4)))) = 0;
#endif

  // setup debug output or silence device
#if (VERBOSE)
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
#else
  // mute logs completely by redirecting them to silence function
  esp_log_level_set("*", ESP_LOG_NONE);
#endif

  // load device configuration from NVRAM and set runmode
  do_after_reset();

  // hash 6 byte device MAC to 4 byte clientID
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  const uint32_t hashedmac = myhash((const char *)mac, 6);
  snprintf(clientId, 20, "paxcounter_%08x", hashedmac);
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
  AXP192_init();
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
// use simple LED for power display if we have additional RGB LED, else for status
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

  // start local webserver on rcommand request
  if (RTC_runmode == RUNMODE_MAINTENANCE)
    start_boot_menu();

#if ((WIFICOUNTER) || (BLECOUNTER))
  // use libpax timer to trigger cyclic senddata
  ESP_LOGI(TAG, "Starting libpax...");
  struct libpax_config_t configuration;
  libpax_default_config(&configuration);

  // configure WIFI sniffing
  strcpy(configuration.wifi_my_country_str, WIFI_MY_COUNTRY);
  configuration.wificounter = cfg.wifiscan;
  configuration.wifi_channel_map = WIFI_CHANNEL_ALL;
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
#else
  // use stand alone timer to trigger cyclic senddata
  initSendDataTimer(cfg.sendcycle * 2);
#endif

#if (BLECOUNTER)
  strcat_P(features, " BLE");
#endif

  // start rcommand processing task
  ESP_LOGI(TAG, "Starting rcommand interpreter...");
  rcmd_init();

// initialize gps
#if (HAS_GPS)
  strcat_P(features, " GPS");
  if (gps_init()) {
    ESP_LOGI(TAG, "Starting GPS Feed...");
    xTaskCreatePinnedToCore(gps_loop,  // task function
                            "gpsloop", // name of task
                            8192,      // stack size of task
                            (void *)1, // parameter of the task
                            1,         // priority of the task
                            &GpsTask,  // task handle
                            1);        // CPU core
  }
#endif

// initialize sensors
#if (HAS_SENSORS)
#if (HAS_SENSOR_1)
  strcat_P(features, " SENS(1)");
  sensor_init();
#endif
#if (HAS_SENSOR_2)
  strcat_P(features, " SENS(2)");
  sensor_init();
#endif
#if (HAS_SENSOR_3)
  strcat_P(features, " SENS(3)");
  sensor_init();
#endif
#endif

// initialize LoRa
#if (HAS_LORA)
  strcat_P(features, " LORA");
  _ASSERT(lmic_init() == ESP_OK);
#endif

// initialize SPI
#ifdef HAS_SPI
  strcat_P(features, " SPI");
  _ASSERT(spi_init() == ESP_OK);
#endif

// initialize MQTT
#ifdef HAS_MQTT
  strcat_P(features, " MQTT");
  _ASSERT(mqtt_init() == ESP_OK);
#endif

#if (HAS_SDCARD)
  if (sdcard_init())
    strcat_P(features, " SD");
#endif

#if (HAS_SDS011)
  ESP_LOGI(TAG, "init fine-dust-sensor");
  if (sds011_init())
    strcat_P(features, " SDS");
#endif

// initialize matrix display
#ifdef HAS_MATRIX_DISPLAY
  strcat_P(features, " LED_MATRIX");
  MatrixDisplayIsOn = cfg.screenon;
  init_matrix_display(); // note: blocking call
#endif

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
  _ASSERT(rtc_init());
#endif

#if defined HAS_DCF77
  strcat_P(features, " DCF77");
#endif

#if defined HAS_IF482
  strcat_P(features, " IF482");
#endif

#if (WIFICOUNTER)
  strcat_P(features, " WIFI");
#else
  // remove wifi driver from RAM, if option wifi not compiled
  esp_wifi_deinit();
#endif

  // start state machine
  ESP_LOGI(TAG, "Starting Interrupt Handler...");
  xTaskCreatePinnedToCore(irqHandler,      // task function
                          "irqhandler",    // name of task
                          4096,            // stack size of task
                          (void *)1,       // parameter of the task
                          4,               // priority of the task
                          &irqHandlerTask, // task handle
                          1);              // CPU core

// initialize BME sensor (BME280/BME680)
#if (HAS_BME)
#ifdef HAS_BME680
  strcat_P(features, " BME680");
#elif defined HAS_BME280
  strcat_P(features, " BME280");
#elif defined HAS_BMP180
  strcat_P(features, " BMP180");
#endif
  if (bme_init())
    ESP_LOGI(TAG, "BME sensor initialized");
  else {
    ESP_LOGE(TAG, "BME sensor could not be initialized");
    cfg.payloadmask &= (uint8_t)~MEMS_DATA; // switch off transmit of BME data
  }
#endif

  // starting timers and interrupts
  _ASSERT(irqHandlerTask != NULL); // has interrupt handler task started?
  ESP_LOGI(TAG, "Starting Timers...");

// display interrupt
#ifdef HAS_DISPLAY
  dp_clear();
  dp_contrast(DISPLAYCONTRAST);
  // https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
  // prescaler 80 -> divides 80 MHz CPU freq to 1 MHz, timer 0, count up
  displayIRQ = timerBegin(0, 80, true);
  timerAttachInterrupt(displayIRQ, &DisplayIRQ, false);
  timerAlarmWrite(displayIRQ, DISPLAYREFRESH_MS * 1000, true);
  timerAlarmEnable(displayIRQ);
#endif

// LED Matrix display interrupt
#ifdef HAS_MATRIX_DISPLAY
  // https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
  // prescaler 80 -> divides 80 MHz CPU freq to 1 MHz, timer 3, count up
  matrixDisplayIRQ = timerBegin(3, 80, true);
  timerAttachInterrupt(matrixDisplayIRQ, &MatrixDisplayIRQ, false);
  timerAlarmWrite(matrixDisplayIRQ, MATRIX_DISPLAY_SCAN_US, true);
  timerAlarmEnable(matrixDisplayIRQ);
#endif

// initialize button
#ifdef HAS_BUTTON
  strcat_P(features, " BTN_");
#ifdef BUTTON_PULLUP
  strcat_P(features, "PU");
#else
  strcat_P(features, "PD");
#endif // BUTTON_PULLUP
  button_init();
#endif // HAS_BUTTON

// only if we have a timesource we do timesync
#if ((HAS_LORA_TIME) || (HAS_GPS) || (HAS_RTC))
  time_init();
  strcat_P(features, " TIME");
#endif // timesync

  // cyclic function interrupts
  cyclicTimer.attach(HOMECYCLE, setCyclicIRQ);

  // show compiled features
  ESP_LOGI(TAG, "Features:%s", features);

  // set runmode to normal
  RTC_runmode = RUNMODE_NORMAL;

  vTaskDelete(NULL);
} // setup()

void loop() { vTaskDelete(NULL); }
