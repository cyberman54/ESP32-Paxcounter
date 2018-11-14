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
wifiloop      0     4     rotates wifi channels
ledloop       0     3     blinks LEDs
spiloop       0     2     reads/writes data on spi interface
IDLE          0     0     ESP32 arduino scheduler -> runs wifi sniffer

looptask      1     1     arduino core -> runs the LMIC LoRa stack
irqhandler    1     1     executes tasks triggered by irq
gpsloop       1     2     reads data from GPS over serial or i2c
bmeloop       1     2     reads data from BME680 over i2c
IDLE          1     0     ESP32 arduino scheduler

ESP32 hardware timers
==========================
 0	Trigger display refresh
 1	Trigger Wifi channel switch
 2	Trigger send payload cycle
 3	Trigger housekeeping cycle

*/

// Basic Config
#include "main.h"

configData_t cfg; // struct holds current device configuration
char display_line6[16], display_line7[16]; // display buffers
uint8_t volatile channel = 0;              // channel rotation counter
uint16_t volatile macs_total = 0, macs_wifi = 0, macs_ble = 0,
                  batt_voltage = 0; // globals for display

hw_timer_t *channelSwitch = NULL, *sendCycle = NULL, *homeCycle = NULL, *displaytimer = NULL; // irq tasks
TaskHandle_t irqHandlerTask, wifiSwitchTask;

std::set<uint16_t> macs; // container holding unique MAC adress hashes

// initialize payload encoder
PayloadConvert payload(PAYLOAD_BUFFER_SIZE);

// local Tag for logging
static const char TAG[] = "main";

void setup() {

  // disable the default wifi logging
  esp_log_level_set("wifi", ESP_LOG_NONE);

  char features[100] = "";

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

  // read (and initialize on first run) runtime settings from NVRAM
  loadConfig(); // includes initialize if necessary

  // initialize leds
#if (HAS_LED != NOT_A_PIN)
  pinMode(HAS_LED, OUTPUT);
  strcat_P(features, " LED");
// switch on power LED if we have 2 LEDs, else use it for status
#ifdef HAS_RGB_LED
  switch_LED(LED_ON);
#endif
#endif

#ifdef HAS_RGB_LED
  rgb_set_color(COLOR_PINK);
  strcat_P(features, " RGB");
#endif

  // initialize wifi antenna
#ifdef HAS_ANTENNA_SWITCH
  strcat_P(features, " ANT");
  antenna_init();
  antenna_select(cfg.wifiant);
#endif

// switch off bluetooth, if not compiled
#ifdef BLECOUNTER
  strcat_P(features, " BLE");
#else
  bool btstop = btStop();
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
#endif

// initialize gps
#ifdef HAS_BME
  strcat_P(features, " BME");
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
  ESP_LOGI(TAG, "ESP32 SDK: %s", ESP.getSdkVersion());
  ESP_LOGI(TAG, "Free RAM: %d bytes", ESP.getFreeHeap());

#ifdef HAS_GPS
  ESP_LOGI(TAG, "TinyGPS+ v%s", TinyGPSPlus::libraryVersion());
#endif

#endif // verbose

// initialize display
#ifdef HAS_DISPLAY
  strcat_P(features, " OLED");
  DisplayState = cfg.screenon;
  init_display(PRODUCTNAME, PROGVERSION);

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

  // setup channel rotation trigger IRQ using esp32 hardware timer 1
  channelSwitch = timerBegin(1, 800, true);
  timerAttachInterrupt(channelSwitch, &ChannelSwitchIRQ, true);
  timerAlarmWrite(channelSwitch, cfg.wifichancycle * 1000, true);

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

  // show compiled features
  ESP_LOGI(TAG, "Features:%s", features);

#ifdef HAS_LORA
  // output LoRaWAN keys to console
#ifdef VERBOSE
  showLoraKeys();
#endif
#endif

// start BLE scan callback if BLE function is enabled in NVRAM configuration
#ifdef BLECOUNTER
  if (cfg.blescan) {
    ESP_LOGI(TAG, "Starting Bluetooth...");
    start_BLEscan();
  }
#endif

  // start wifi in monitor mode and start channel rotation task on core 0
  ESP_LOGI(TAG, "Starting Wifi...");
  wifi_sniffer_init();
  // initialize salt value using esp_random() called by random() in
  // arduino-esp32 core. Note: do this *after* wifi has started, since
  // function gets it's seed from RF noise
  get_salt(); // get new 16bit for salting hashes

#ifdef HAS_GPS
  ESP_LOGI(TAG, "Starting GPSloop...");
  xTaskCreatePinnedToCore(gps_loop,  // task function
                          "gpsloop", // name of task
                          2048,      // stack size of task
                          (void *)1, // parameter of the task
                          2,         // priority of the task
                          &GpsTask,  // task handle
                          1);        // CPU core
#endif

#ifdef HAS_BME
  ESP_LOGI(TAG, "Starting BMEloop...");
  xTaskCreatePinnedToCore(bme_loop,  // task function
                          "bmeloop", // name of task
                          2048,      // stack size of task
                          (void *)1, // parameter of the task
                          2,         // priority of the task
                          &BmeTask,  // task handle
                          1);        // CPU core
#endif

  // start state machine
  ESP_LOGI(TAG, "Starting IRQ Handler...");
  xTaskCreatePinnedToCore(irqHandler,      // task function
                          "irqhandler",    // name of task
                          2048,            // stack size of task
                          (void *)1,       // parameter of the task
                          1,               // priority of the task
                          &irqHandlerTask, // task handle
                          1);              // CPU core

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  // start led loop
  ESP_LOGI(TAG, "Starting LEDloop...");
  xTaskCreatePinnedToCore(ledLoop,      // task function
                          "ledloop",    // name of task
                          1024,         // stack size of task
                          (void *)1,    // parameter of the task
                          3,            // priority of the task
                          &ledLoopTask, // task handle
                          0);           // CPU core
#endif

  // start wifi channel rotation task
  ESP_LOGI(TAG, "Starting Wifi Channel rotation...");
  xTaskCreatePinnedToCore(switchWifiChannel, // task function
                          "wifiloop",        // name of task
                          2048,              // stack size of task
                          NULL,              // parameter of the task
                          4,                 // priority of the task
                          &wifiSwitchTask,   // task handle
                          0);                // CPU core

  // start timer triggered interrupts
  ESP_LOGI(TAG, "Starting Interrupts...");
#ifdef HAS_DISPLAY
  timerAlarmEnable(displaytimer);
#endif
  timerAlarmEnable(sendCycle);
  timerAlarmEnable(homeCycle);
  timerAlarmEnable(channelSwitch);

  // start button interrupt
#ifdef HAS_BUTTON
#ifdef BUTTON_PULLUP
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, RISING);
#else
  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), ButtonIRQ, FALLING);
#endif
#endif // HAS_BUTTON

} // setup()

void loop() {

  while (1) {
#ifdef HAS_LORA
    os_runloop_once(); // execute lmic scheduled jobs and events
#endif
    vTaskDelay(2 / portTICK_PERIOD_MS); // yield to CPU
  }

  vTaskDelete(NULL); // shoud never be reached
}