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
#include "main.h"

configData_t cfg; // struct holds current device configuration
char display_line6[16], display_line7[16]; // display buffers
uint8_t channel = 0;                       // channel rotation counter
uint16_t macs_total = 0, macs_wifi = 0, macs_ble = 0,
         batt_voltage = 0; // globals for display

// hardware timer for cyclic tasks
hw_timer_t *channelSwitch, *displaytimer, *sendCycle, *homeCycle;

// this variables will be changed in the ISR, and read in main loop
volatile int ButtonPressedIRQ = 0, ChannelTimerIRQ = 0, SendCycleTimerIRQ = 0,
             DisplayTimerIRQ = 0, HomeCycleIRQ = 0;

TaskHandle_t WifiLoopTask = NULL;
TaskHandle_t StateTask = NULL;

// RTos send queues for payload transmit
#ifdef HAS_LORA
QueueHandle_t LoraSendQueue;
TaskHandle_t LoraTask = NULL;
#endif

#ifdef HAS_SPI
QueueHandle_t SPISendQueue;
#endif

#ifdef HAS_GPS
TaskHandle_t GpsTask = NULL;
#endif

portMUX_TYPE timerMux =
    portMUX_INITIALIZER_UNLOCKED; // sync main loop and ISR when modifying IRQ
                                  // handler shared variables

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

  ESP_LOGI(TAG, "Starting %s v%s", PRODUCTNAME, PROGVERSION);

  // initialize system event handler for wifi task, needed for
  // wifi_sniffer_init()
  // esp_event_loop_init(NULL, NULL);
  // ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

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

  // reboot to firmware update mode if ota trigger switch is set
  if (cfg.runmode == 1) {
    cfg.runmode = 0;
    saveConfig();
    start_ota_update();
  }

#ifdef VENDORFILTER
  strcat_P(features, " OUIFLT");
#endif

// initialize LoRa
#ifdef HAS_LORA
  strcat_P(features, " LORA");
  LoraSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (LoraSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create LORA send queue. Aborting.");
    exit(0);
  } else
    ESP_LOGI(TAG, "LORA send queue created, size %d Bytes",
             SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);
#endif

// initialize SPI
#ifdef HAS_SPI
  strcat_P(features, " SPI");
  SPISendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (SPISendQueue == 0) {
    ESP_LOGE(TAG, "Could not create SPI send queue. Aborting.");
    exit(0);
  } else
    ESP_LOGI(TAG, "SPI send queue created, size %d Bytes",
             SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);
#endif

    // initialize led
#if (HAS_LED != NOT_A_PIN)
  pinMode(HAS_LED, OUTPUT);
  strcat_P(features, " LED");
#endif

#ifdef HAS_RGB_LED
  rgb_set_color(COLOR_PINK);
  strcat_P(features, " RGB");
#endif

  // initialize button
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

  // initialize wifi antenna
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

// initialize gps
#ifdef HAS_GPS
  strcat_P(features, " GPS");
#endif

// initialize battery status
#ifdef HAS_BATTERY_PROBE
  strcat_P(features, " BATT");
  calibrate_voltage();
  batt_voltage = read_voltage();
#endif

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
  // enable display interrupt
  yield();
  timerAlarmEnable(displaytimer);
#endif

  // setup channel rotation trigger IRQ using esp32 hardware timer 1
  channelSwitch = timerBegin(1, 800, true);
  timerAttachInterrupt(channelSwitch, &ChannelSwitchIRQ, true);
  timerAlarmWrite(channelSwitch, cfg.wifichancycle * 1000, true);

  // setup send cycle trigger IRQ using esp32 hardware timer 2
  sendCycle = timerBegin(2, 8000, true);
  timerAttachInterrupt(sendCycle, &SendCycleIRQ, true);
  timerAlarmWrite(sendCycle, cfg.sendcycle * 2 * 10000, true);

  // setup house keeping cycle trigger IRQ using esp32 hardware timer 3
  homeCycle = timerBegin(3, 8000, true);
  timerAttachInterrupt(homeCycle, &homeCycleIRQ, true);
  timerAlarmWrite(homeCycle, HOMECYCLE * 10000, true);

  // enable timers
  // caution, see: https://github.com/espressif/arduino-esp32/issues/1313
  yield();
  timerAlarmEnable(homeCycle);
  yield();
  timerAlarmEnable(sendCycle);
  yield();
  timerAlarmEnable(channelSwitch);

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

  // initialize LoRaWAN LMIC run-time environment
  os_init();
  // reset LMIC MAC state
  LMIC_reset();
  // This tells LMIC to make the receive windows bigger, in case your clock is
  // 1% faster or slower.
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  // join network
  LMIC_startJoining();

  /*

  Overview Tasks & Timer

  Task          Core  Prio  Purpose
  ====================================================================
  IDLE          0     0     ESP32 arduino scheduler
  wifiloop      0     1     switches Wifi channels
  gpsloop       0     2     reasd data from GPS over serial or i2c
  IDLE          1     0     Arduino loop() -> unused
  loraloop      1     1     runs the LMIC stack
  statemachine  1     3     switches process logic

  Timers
  ======
  0	Display-Refresh
  1	Wifi Channel Switch
  2	Send Cycle
  3	Housekeeping

  */

  // start lmic runloop in rtos task on core 1
  // (note: arduino main loop runs on core 1, too)
  // https://techtutorialsx.com/2017/05/09/esp32-get-task-execution-core/

  ESP_LOGI(TAG, "Starting Lora...");
  xTaskCreatePinnedToCore(lorawan_loop, "loraloop", 2048, (void *)1, 1,
                          &LoraTask, 1);
#endif

// if device has GPS and it is enabled, start GPS reader task on core 0 with
// higher priority than wifi channel rotation task since we process serial
// streaming NMEA data
#ifdef HAS_GPS
  ESP_LOGI(TAG, "Starting GPS...");
  xTaskCreatePinnedToCore(gps_loop, "gpsloop", 2048, (void *)1, 2, &GpsTask, 0);
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
  reset_salt(); // get new 16bit for salting hashes
  xTaskCreatePinnedToCore(wifi_channel_loop, "wifiloop", 2048, (void *)1, 1,
                          &WifiLoopTask, 0);

  // start state machine
  ESP_LOGI(TAG, "Starting Statemachine...");
  xTaskCreatePinnedToCore(stateMachine, "stateloop", 2048, (void *)1, 3,
                          &StateTask, 1);

} // setup()

void stateMachine(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
    led_loop();
#endif

#ifdef HAS_BUTTON
    readButton();
#endif

#ifdef HAS_DISPLAY
    updateDisplay();
#endif

    // check housekeeping cycle and if expired do the work
    if (HomeCycleIRQ)
      doHousekeeping();
    // check send queue and process it
    processSendBuffer();
    // check send cycle and enqueue payload if cycle is expired
    if (SendCycleTimerIRQ)
      sendPayload();
    // yield to CPU
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}

void loop() { vTaskDelay(2 / portTICK_PERIOD_MS); }
