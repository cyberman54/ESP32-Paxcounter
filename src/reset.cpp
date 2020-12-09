// Basic Config
#include "globals.h"
#include "reset.h"

// Local logging tag
static const char TAG[] = __FILE__;

// Conversion factor for micro seconds to seconds
#define uS_TO_S_FACTOR 1000000ULL

// variable keep its values after restart or wakeup from sleep
RTC_NOINIT_ATTR runmode_t RTC_runmode;

const char *runmode[4] = {"powercycle", "normal", "wakeup", "update"};

void do_reset(bool warmstart) {
  if (warmstart) {
    ESP_LOGI(TAG, "restarting device (warmstart), keeping runmode %s",
             runmode[RTC_runmode]);
  } else {
#if (HAS_LORA)
    if (RTC_runmode == RUNMODE_NORMAL) {
      LMIC_shutdown();
    }
#endif
    RTC_runmode = RUNMODE_POWERCYCLE;
    ESP_LOGI(TAG, "restarting device (coldstart), setting runmode %s",
             runmode[RTC_runmode]);
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

  ESP_LOGI(TAG, "Starting Software v%s, runmode %s", PROGVERSION,
           runmode[RTC_runmode]);
}

void enter_deepsleep(const int wakeup_sec = 60,
                     const gpio_num_t wakeup_gpio = GPIO_NUM_MAX) {

  // ensure we are in normal runmode, not udpate or wakeup
  if ((RTC_runmode != RUNMODE_NORMAL)
#if (HAS_LORA)
      || (LMIC.opmode & (OP_JOINING | OP_REJOIN))
#endif
  ) {
    ESP_LOGE(TAG, "Can't go to sleep now");
    return;
  } else {
    ESP_LOGI(TAG, "Attempting to sleep...");
  }

  // switch off radio
#if (BLECOUNTER)
  stop_BLEscan();
  btStop();
#endif
#if (WIFICOUNTER)
  switch_wifi_sniffer(0);
#endif

  // wait until all send queues are empty
  ESP_LOGI(TAG, "Waiting until send queues are empty...");
  while (!allQueuesEmtpy())
    vTaskDelay(pdMS_TO_TICKS(100));

#if (HAS_LORA)
  // shutdown LMIC safely
  ESP_LOGI(TAG, "Waiting until LMIC is idle...");
  while ((LMIC.opmode & OP_TXRXPEND) ||
         os_queryTimeCriticalJobs(sec2osticks(wakeup_sec)))
    vTaskDelay(pdMS_TO_TICKS(100));

  SaveLMICToRTC(wakeup_sec);
// vTaskDelete(lmicTask);
// LMIC_shutdown();
#endif // (HAS_LORA)

  // set up RTC power domains
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

  // set up RTC wakeup timer, if we have
  if (wakeup_sec > 0) {
    esp_sleep_enable_timer_wakeup(wakeup_sec * uS_TO_S_FACTOR);
  }

  // set wakeup gpio, if we have
  if (wakeup_gpio != GPIO_NUM_MAX) {
    rtc_gpio_isolate(wakeup_gpio);
    esp_sleep_enable_ext1_wakeup(1ULL << wakeup_gpio, ESP_EXT1_WAKEUP_ALL_LOW);
  }

  // halt interrupts accessing i2c bus
  mask_user_IRQ();

// switch off display
#ifdef HAS_DISPLAY
  dp_shutdown();
#endif

// reduce power if has PMU
#ifdef HAS_PMU
  AXP192_power(pmu_power_sleep);
#endif

  // shutdown i2c bus
  i2c_deinit();

  // enter sleep mode
  ESP_LOGI(TAG, "Going to sleep, good bye.");
  esp_deep_sleep_start();
}