// Basic Config
#include "globals.h"
#include "reset.h"

// Local logging tag
static const char TAG[] = __FILE__;

// variable keep its values after restart or wakeup from sleep
RTC_NOINIT_ATTR runmode_t RTC_runmode;

void do_reset(bool warmstart) {
  if (warmstart) {
    // store LMIC keys and counters in RTC memory
    ESP_LOGI(TAG, "restarting device (warmstart), keeping runmode %d",
             RTC_runmode);
  } else {
#if (HAS_LORA)
    if (RTC_runmode == RUNMODE_NORMAL)
      LMIC_shutdown();
#endif
    RTC_runmode = RUNMODE_POWERCYCLE;
    ESP_LOGI(TAG, "restarting device (coldstart), set runmode %d", RTC_runmode);
  }
  esp_restart();
}

void do_after_reset(int reason) {

  switch (reason) {

  case POWERON_RESET:          // 0x01 Vbat power on reset
  case RTCWDT_BROWN_OUT_RESET: // 0x0f Reset when the vdd voltage is not
                               // stable
    RTC_runmode = RUNMODE_POWERCYCLE;
    break;

  case SW_CPU_RESET: // 0x0c Software reset CPU
                     // keep previous runmode (could be RUNMODE_UPDATE)
    break;

  case DEEPSLEEP_RESET: // 0x05 Deep Sleep reset digital core
    RTC_runmode = RUNMODE_WAKEUP;
#if (HAS_LORA)
    // to be done: restore LoRaWAN channel configuration and datarate here
#endif
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
    break;
  }

  ESP_LOGI(TAG, "Starting Software v%s, runmode %d", PROGVERSION, RTC_runmode);
}

void enter_deepsleep(const int wakeup_sec, const gpio_num_t wakeup_gpio) {

  if ((!wakeup_sec) && (!wakeup_gpio) && (RTC_runmode == RUNMODE_NORMAL))
    return;

// assure LMIC is in safe state
#if (HAS_LORA)
  if (os_queryTimeCriticalJobs(ms2osticks(10000)))
    return;

    // to be done: save LoRaWAN channel configuration here

#endif

  // set up power domains
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

  // set wakeup timer
  if (wakeup_sec)
    esp_sleep_enable_timer_wakeup(wakeup_sec * 1000000);

  // set wakeup gpio
  if (wakeup_gpio != NOT_A_PIN) {
    rtc_gpio_isolate(wakeup_gpio);
    esp_sleep_enable_ext1_wakeup(1ULL << wakeup_gpio, ESP_EXT1_WAKEUP_ALL_LOW);
  }

  // halt interrupts accessing i2c bus
  mask_user_IRQ();

// switch off display
#ifdef HAS_DISPLAY
  shutdown_display();
#endif

// switch off wifi & ble
#if (BLECOUNTER)
  stop_BLEscan();
#endif

// reduce power if has PMU
#ifdef HAS_PMU
  AXP192_power(pmu_power_sleep);
#endif

  // shutdown i2c bus
  i2c_deinit();

  // enter sleep mode
  ESP_LOGI(TAG, "Going to sleep...");
  esp_deep_sleep_start();
}