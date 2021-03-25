// Basic Config
#include "globals.h"
#include "reset.h"

// Local logging tag
static const char TAG[] = __FILE__;

// Conversion factor for micro seconds to seconds
#define uS_TO_S_FACTOR 1000000ULL

// RTC_NOINIT_ATTR -> keep value after a software restart or system crash
RTC_NOINIT_ATTR runmode_t RTC_runmode;
RTC_NOINIT_ATTR uint32_t RTC_restarts;

// RTC_DATA_ATTR -> keep values after a wakeup from sleep
RTC_DATA_ATTR struct timeval RTC_sleep_start_time;
RTC_DATA_ATTR unsigned long long RTC_millis = 0;

timeval sleep_stop_time;

void reset_rtc_vars(void) {
  RTC_runmode = RUNMODE_POWERCYCLE;
  RTC_restarts = 0;
}

void do_reset(bool warmstart) {
  if (warmstart) {
    ESP_LOGI(TAG, "restarting device (warmstart)");
  } else {
#if (HAS_LORA)
    if (RTC_runmode == RUNMODE_NORMAL) {
      LMIC_shutdown();
    }
#endif
    RTC_runmode = RUNMODE_POWERCYCLE;
    ESP_LOGI(TAG, "restarting device (coldstart)");
  }
  esp_restart();
}

void do_after_reset(void) {

  struct timeval sleep_stop_time;
  uint64_t sleep_time_ms;

  // read (and initialize on first run) runtime settings from NVRAM
  loadConfig();

  switch (rtc_get_reset_reason(0)) {

  case POWERON_RESET:          // 0x01 Vbat power on reset
  case RTCWDT_BROWN_OUT_RESET: // 0x0f Reset when the vdd voltage is not
                               // stable
    reset_rtc_vars();
    break;

  case SW_CPU_RESET: // 0x0c Software reset CPU
    // keep previous set runmode (update / normal / maintenance)
    RTC_restarts++;
    break;

  case DEEPSLEEP_RESET: // 0x05 Deep Sleep reset digital core
    // calculate time spent in deep sleep
    gettimeofday(&sleep_stop_time, NULL);
    sleep_time_ms =
        (sleep_stop_time.tv_sec - RTC_sleep_start_time.tv_sec) * 1000 +
        (sleep_stop_time.tv_usec - RTC_sleep_start_time.tv_usec) / 1000;
    ESP_LOGI(TAG, "Time spent in deep sleep: %d ms", sleep_time_ms);
    RTC_millis += sleep_time_ms; // increment system monotonic time
    // set wakeup state, not if we have pending OTA update
    if (RTC_runmode == RUNMODE_SLEEP)
      RTC_runmode = RUNMODE_WAKEUP;
    break;

  case SW_RESET:         // 0x03 Software reset digital core
  case OWDT_RESET:       // 0x04 Legacy watch dog reset digital core
  case SDIO_RESET:       // 0x06 Reset by SLC module, reset digital core
  case TG0WDT_SYS_RESET: // 0x07 Timer Group0 Watch dog reset digital core
  case TG1WDT_SYS_RESET: // 0x08 Timer Group1 Watch dog reset digital core
  case RTCWDT_SYS_RESET: // 0x09 RTC Watch dog Reset digital core
  case INTRUSION_RESET:  // 0x0a Instrusion tested to reset CPU
  case TGWDT_CPU_RESET:  // 0x0b Time Group reset CPU
  case RTCWDT_CPU_RESET: // 0x0d RTC Watch dog Reset CPU
  case EXT_CPU_RESET:    // 0x0e for APP CPU, reseted by PRO CPU
  case RTCWDT_RTC_RESET: // 0x10 RTC Watch dog reset digital core and rtc mode
  default:
    RTC_runmode = RUNMODE_POWERCYCLE;
    RTC_restarts++;
    break;
  }

  ESP_LOGI(TAG, "Starting Software v%s (runmode=%d / restarts=%d)", PROGVERSION,
           RTC_runmode, RTC_restarts);
}

void enter_deepsleep(const uint64_t wakeup_sec, gpio_num_t wakeup_gpio) {

  ESP_LOGI(TAG, "Preparing to sleep...");

  RTC_runmode = RUNMODE_SLEEP;
  int i;

  // validate wake up pin, if we have
  if (!GPIO_IS_VALID_GPIO(wakeup_gpio))
    wakeup_gpio = GPIO_NUM_MAX;

  // stop further enqueuing of senddata and MAC processing
  sendTimer.detach();

  // switch off radio and other power consuming hardware
#if (WIFICOUNTER)
  switch_wifi_sniffer(0);
#endif
#if (BLECOUNTER)
  stop_BLEscan();
  btStop();
#endif
#if (HAS_SDS011)
  sds011_sleep(void);
#endif

  // stop MAC processing
  vTaskDelete(macProcessTask);

  // halt interrupts accessing i2c bus
  mask_user_IRQ();

  // wait a while (max 100 sec) to clear send queues
  ESP_LOGI(TAG, "Waiting until send queues are empty...");
  for (i = 100; i > 0; i--) {
    if (allQueuesEmtpy())
      break;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // shutdown LMIC safely, waiting max 100 sec
#if (HAS_LORA)
  ESP_LOGI(TAG, "Waiting until LMIC is idle...");
  for (i = 100; i > 0; i--) {
    if ((LMIC.opmode & OP_TXRXPEND) ||
        os_queryTimeCriticalJobs(sec2osticks(wakeup_sec)))
      vTaskDelay(pdMS_TO_TICKS(1000));
    else
      break;
  }
#endif // (HAS_LORA)

// shutdown MQTT safely
#ifdef HAS_MQTT
  mqtt_deinit();
#endif

// shutdown SPI safely
#ifdef HAS_SPI
  spi_deinit();
#endif

// save LMIC state to RTC RAM
#if (HAS_LORA)
  SaveLMICToRTC(wakeup_sec);
#endif // (HAS_LORA)

// set display to power save mode
#ifdef HAS_DISPLAY
  dp_shutdown();
#endif

// reduce power if has PMU or VEXT
#ifdef HAS_PMU
  AXP192_power(pmu_power_sleep);
#elif EXT_POWER_SW
  digitalWrite(EXT_POWER_SW, EXT_POWER_OFF);
#endif

  // shutdown i2c bus
  i2c_deinit();

  // configure wakeup sources
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html

  // set up RTC wakeup timer, if we have
  if (wakeup_sec > 0) {
    esp_sleep_enable_timer_wakeup(wakeup_sec * uS_TO_S_FACTOR);
  }

  // set wakeup gpio, if we have
  if (wakeup_gpio != GPIO_NUM_MAX) {
    rtc_gpio_isolate(wakeup_gpio); // minimize deep sleep current
    esp_sleep_enable_ext1_wakeup(1ULL << wakeup_gpio, ESP_EXT1_WAKEUP_ALL_LOW);
  }

  // time stamp sleep start time and save system monotonic time. Deep sleep.
  gettimeofday(&RTC_sleep_start_time, NULL);
  RTC_millis += millis();
  ESP_LOGI(TAG, "Going to sleep, good bye.");
  esp_deep_sleep_start();
}

unsigned long long uptime() { return (RTC_millis + millis()); }