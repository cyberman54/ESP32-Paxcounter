// Basic Config
#include "globals.h"
#include "reset.h"

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
    reset_rtc_vars();
    ESP_LOGI(TAG, "restarting device (coldstart)");
  }
  esp_restart();
}

void do_after_reset(void) {
  struct timeval sleep_stop_time;
  uint64_t sleep_time_ms;

  // read (and initialize on first run) runtime settings from NVRAM
  loadConfig();

  // set time zone to user value from paxcounter.conf
#ifdef TIME_SYNC_TIMEZONE
  setenv("TZ", TIME_SYNC_TIMEZONE, 1);
  tzset();
  ESP_LOGD(TAG, "Timezone set to %s", TIME_SYNC_TIMEZONE);
#endif

  switch (rtc_get_reset_reason(0)) {
  case RESET_REASON_CHIP_POWER_ON:
  case RESET_REASON_SYS_BROWN_OUT:
    reset_rtc_vars();
    break;

  case RESET_REASON_CPU0_SW:
    // keep previous set runmode (update / normal / maintenance)
    RTC_restarts++;
    break;

  case RESET_REASON_CORE_DEEP_SLEEP:
    // calculate time spent in deep sleep
    gettimeofday(&sleep_stop_time, NULL);
    sleep_time_ms =
        (sleep_stop_time.tv_sec - RTC_sleep_start_time.tv_sec) * 1000 +
        (sleep_stop_time.tv_usec - RTC_sleep_start_time.tv_usec) / 1000;
    RTC_millis += sleep_time_ms; // increment system monotonic time
    ESP_LOGI(TAG, "Time spent in deep sleep: %d ms", sleep_time_ms);
    // do we have a valid time? -> set global variable
    timeSource = timeIsValid(sleep_stop_time.tv_sec) ? _set : _unsynced;
    // set wakeup state, not if we have pending OTA update
    if (RTC_runmode == RUNMODE_SLEEP)
      RTC_runmode = RUNMODE_WAKEUP;
    break;

  default:
    RTC_runmode = RUNMODE_POWERCYCLE;
    RTC_restarts++;
    break;
  }
}

void enter_deepsleep(const uint32_t wakeup_sec, gpio_num_t wakeup_gpio) {
  ESP_LOGI(TAG, "Preparing to sleep...");

  RTC_runmode = RUNMODE_SLEEP;

  // validate wake up pin, if we have
  if (!GPIO_IS_VALID_GPIO(wakeup_gpio))
    wakeup_gpio = GPIO_NUM_MAX;
  // stop further enqueuing of senddata and MAC processing
  libpax_counter_stop();

  // switch off any power consuming hardware
#if (HAS_SDS011)
  sds011_sleep();
#endif

  // wait a while (max 100 sec) to clear send queues
  ESP_LOGI(TAG, "Waiting until send queues are empty...");
  for (int i = 100; i > 0; i--) {
    if (allQueuesEmtpy())
      break;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

// wait up to 100secs until LMIC is idle
#if (HAS_LORA)
  lora_waitforidle(100);
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

  // halt interrupts accessing i2c bus
  mask_user_IRQ();

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
  RTC_millis += esp_timer_get_time() / 1000;
  ESP_LOGI(TAG, "Going to sleep, good bye.");

// flush & close sd card, if we have
#if (HAS_SDCARD)
  sdcard_close();
#endif

  esp_deep_sleep_start();
}

unsigned long long uptime() {
  return (RTC_millis + esp_timer_get_time() / 1000);
}