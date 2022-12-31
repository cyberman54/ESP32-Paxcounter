/* This routine is called by interrupt in regular intervals */
/* Interval can be set in paxcounter.conf (HOMECYCLE)       */

// Basic config
#include "cyclic.h"


Ticker cyclicTimer;

void setCyclicIRQ() { xTaskNotify(irqHandlerTask, CYCLIC_IRQ, eSetBits); }

// do all housekeeping
void doHousekeeping() {
  // check if update or maintenance mode trigger switch was set by rcommand
  if ((RTC_runmode == RUNMODE_UPDATE) || (RTC_runmode == RUNMODE_MAINTENANCE))
    do_reset(true); // warmstart

  // print heap and task storage information
  ESP_LOGD(TAG, "Heap: Free:%d, Min:%d, Size:%d, Alloc:%d, StackHWM:%d",
           ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(),
           ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(NULL));

  if (irqHandlerTask != NULL)
    ESP_LOGD(TAG, "IRQhandler %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(irqHandlerTask),
             eTaskGetState(irqHandlerTask));
  if (rcmdTask != NULL)
    ESP_LOGD(TAG, "Rcommand interpreter %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(rcmdTask), eTaskGetState(rcmdTask));

#if (HAS_LORA)
  if (lmicTask != NULL)
    ESP_LOGD(TAG, "LMiCtask %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(lmicTask), eTaskGetState(lmicTask));
  if (lorasendTask != NULL)
    ESP_LOGD(TAG, "Lorasendtask %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(lorasendTask),
             eTaskGetState(lorasendTask));
#endif

#if (HAS_GPS)
  if (GpsTask != NULL)
    ESP_LOGD(TAG, "Gpsloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(GpsTask), eTaskGetState(GpsTask));
#endif

#ifdef HAS_SPI
  if (spiTask != NULL)
    ESP_LOGD(TAG, "spiloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(spiTask), eTaskGetState(spiTask));
#endif

#ifdef HAS_MQTT
  if (mqttTask != NULL)
    ESP_LOGD(TAG, "MQTTloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(mqttTask), eTaskGetState(mqttTask));
#endif

#if (defined HAS_DCF77 || defined HAS_IF482)
  if (ClockTask != NULL)
    ESP_LOGD(TAG, "Clockloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(ClockTask), eTaskGetState(ClockTask));
#endif

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  if (ledLoopTask != NULL)
    ESP_LOGD(TAG, "LEDloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(ledLoopTask),
             eTaskGetState(ledLoopTask));
#endif

#ifdef HAS_BUTTON
  if (buttonLoopTask != NULL)
    ESP_LOGD(TAG, "Buttonloop %d bytes left | Taskstate = %d",
             uxTaskGetStackHighWaterMark(buttonLoopTask),
             eTaskGetState(buttonLoopTask));
#endif

// read battery voltage into global variable
#if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
  batt_level = read_battlevel();
#ifdef HAS_PMU
  AXP192_showstatus();
#endif
#ifdef HAS_IP5306
  printIP5306Stats();
#endif
#endif

// display BME680/280 sensor data
#if (HAS_BME)
#ifdef HAS_BME680
  ESP_LOGI(TAG, "BME680 Temp: %.2f°C | IAQ: %.2f | IAQacc: %d",
           bme_status.temperature, bme_status.iaq, bme_status.iaq_accuracy);
#elif defined HAS_BME280
  ESP_LOGI(TAG, "BME280 Temp: %.2f°C | Humidity: %.2f | Pressure: %.0f",
           bme_status.temperature, bme_status.humidity, bme_status.pressure);
#elif defined HAS_BMP180
  ESP_LOGI(TAG, "BMP180 Temp: %.2f°C | Pressure: %.0f", bme_status.temperature,
           bme_status.pressure);
#endif
#endif

  // check free heap memory
  if (ESP.getMinFreeHeap() <= MEM_LOW) {
    ESP_LOGW(TAG,
             "Memory full, counter cleared (heap low water mark = %d Bytes / "
             "free heap = %d bytes)",
             ESP.getMinFreeHeap(), ESP.getFreeHeap());
    do_reset(true); // memory leak, reset device
  }

// check free PSRAM memory
#ifdef BOARD_HAS_PSRAM
  if (ESP.getMinFreePsram() <= MEM_LOW) {
    ESP_LOGW(TAG, "PSRAM full, counter cleared");
    do_reset(true); // memory leak, reset device
  }
#endif

#if (HAS_SDS011)
  if (isSDS011Active) {
    sds011_loop();
  } else {
    sds011_wakeup();
  }
#endif

#if (HAS_SDCARD)
  sdcard_flush();
#endif
} // doHousekeeping()

uint32_t getFreeRAM() {
#ifndef BOARD_HAS_PSRAM
  return ESP.getFreeHeap();
#else
  return ESP.getFreePsram();
#endif
}