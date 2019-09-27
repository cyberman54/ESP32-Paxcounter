#ifdef HAS_DISPLAY

/*

Display-Mask (128 x 64 pixel):

 |          111111
 |0123456789012345
------------------
0|PAX:aabbccddee
1|PAX:aabbccddee
2|B:a.bcV  Sats:ab
3|BLTH:abcde  SFab
4|WIFI:abcde ch:ab
5|RLIM:abcd abcdKB
6|20:27:00* 27.Feb
7|yyyyyyyyyyyyyyab

line 6: * = char {L|G|R|?} indicates time source,
            inverse = clock controller is active,
            pulsed = pps input signal is active

line 7: y = LMIC event message; ab = payload queue length

*/

// Basic Config
#include "globals.h"
#include <ss_oled.h>
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

#define DISPLAY_PAGES (4) // number of display pages
#define USE_BACKBUFFER    // for display library

// helper arry for converting month values to text
const char *printmonth[] = {"xxx", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

uint8_t DisplayIsOn = 0;

void init_display(const char *Productname, const char *Version) {

  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {

    // init display
#ifndef DISPLAY_FLIP
    oledInit(OLED_128x64, ANGLE_0, false, -1, -1, 400000L);
#else
    oledInit(OLED_128x64, ANGLE_FLIPY, false, -1, -1, 400000L);
#endif

    // clear display
    oledFill(0, 1);

    // show startup screen
    // to come -> display .bmp file with logo

// show chip information
#if (VERBOSE)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    dp_printf(0, 0, 0, 0, "ESP32 %d cores", chip_info.cores);
    dp_printf(0, 2, 0, 0, "Chip Rev.%d", chip_info.revision);
    dp_printf(0, 1, 0, 0, "WiFi%s%s",
              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    dp_printf(0, 3, 0, 0, "%dMB %s Flash",
              spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
#endif // VERBOSE

#if (HAS_LORA)
    uint8_t buf[34];
    const uint8_t *p;
    os_getDevEui((u1_t *)buf);
    dp_printf(0, 5, 0, 0, "DEVEUI:");
    for (uint8_t i = 0; i < 8; i++) {
      p = buf + 7 - i;
      dp_printf(i * 16, 6, 0, 0, "%02X", *p);
    }
#endif // HAS_LORA

    dp_printf(0, 4, 0, 0, "Software v%s", PROGVERSION);
    delay(3000);

    oledFill(0, 1);
    oledPower(cfg.screenon); // set display off if disabled
    dp_printf(0, 0, 1, 0, "PAX:0");
#if (BLECOUNTER)
    dp_printf(0, 3, 0, 0, "BLTH:0");
#endif
    dp_printf(0, 4, 0, 0, "WIFI:0");
    dp_printf(0, 5, 0, 0, !cfg.rssilimit ? "RLIM:off " : "RLIM:%d",
              cfg.rssilimit);

    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }                     // mutex
} // init_display

void refreshTheDisplay(bool nextPage) {

  static uint8_t DisplayPage = 0;

  // if display is switched off we don't refresh it to relax cpu
  if (!DisplayIsOn && (DisplayIsOn == cfg.screenon))
    return;

  const time_t t =
      myTZ.toLocal(now()); // note: call now() here *before* locking mutex!

  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {
    // set display on/off according to current device configuration
    if (DisplayIsOn != cfg.screenon) {
      DisplayIsOn = cfg.screenon;
      oledPower(cfg.screenon);
    }

    if (nextPage) {
      DisplayPage = (DisplayPage >= DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
      oledFill(0, 1);
    }

    draw_page(t, DisplayPage);

    I2C_MUTEX_UNLOCK(); // release i2c bus access

  } // mutex
} // refreshDisplay()

void draw_page(time_t t, uint8_t page) {

  char timeState, buff[16];
  uint8_t msgWaiting;
#if (HAS_GPS)
  static bool wasnofix = true;
#endif

  // update counter (lines 0-1)
  dp_printf(0, 0, FONT_STRETCHED, 0, "PAX:%-4d",
            (int)macs.size()); // display number of unique macs total Wifi + BLE

  switch (page % DISPLAY_PAGES) {

    // page 0: parameters overview
    // page 1: time
    // page 2: GPS
    // page 3: BME280/680

  case 0:

// update Battery status (line 2)
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
    if (batt_voltage == 0xffff)
      dp_printf(0, 2, 0, 0, "%s", "B:USB  ");
    else
      dp_printf(0, 2, 0, 0, "B:%.2fV", batt_voltage / 1000.0);
#endif

// update GPS status (line 2)
#if (HAS_GPS)
    if (gps.location.age() < 1500) // if no fix then display Sats value inverse
      dp_printf(72, 2, 0, 0, "Sats:%.2d", gps.satellites.value());
    else
      dp_printf(72, 2, 0, 1, "Sats:%.2d", gps.satellites.value());
#endif

      // update bluetooth counter + LoRa SF (line 3)
#if (BLECOUNTER)
    if (cfg.blescan)
      dp_printf(0, 3, 0, 0, "BLTH:%-5d", macs_ble);
    else
      dp_printf(0, 3, 0, 0, "%s", "BLTH:off");
#endif

#if (HAS_LORA)
    if (cfg.adrmode)
      dp_printf(96, 3, 0, 0, "%-4s", getSfName(updr2rps(LMIC.datarate)));
    else // if ADR=off then display SF value inverse
      dp_printf(96, 3, 0, 1, "%-4s", getSfName(updr2rps(LMIC.datarate)));
#endif // HAS_LORA

    // line 4: update wifi counter + channel display
    dp_printf(0, 4, 0, 0, "WIFI:%-5d", macs_wifi);
    dp_printf(88, 4, 0, 0, "ch:%02d", channel);

    // line 5: update RSSI limiter status & free memory display
    dp_printf(0, 5, 0, 0, !cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d",
              cfg.rssilimit);
    dp_printf(80, 5, 0, 0, "%4dKB", getFreeRAM() / 1024);

    // line 6: update time-of-day or LoRa status display
#if (TIME_SYNC_INTERVAL)
    // we want a systime display instead LoRa status
    timeState = TimePulseTick ? ' ' : timeSetSymbols[timeSource];
    TimePulseTick = false;
// display inverse timeState if clock controller is enabled
#if (defined HAS_DCF77) || (defined HAS_IF482)
    dp_printf(0, 6, FONT_SMALL, 0, "%02d:%02d:%02d", hour(t), minute(t),
              second(t));
    dp_printf(56, 6, FONT_SMALL, 1, "%c", timeState);
#else
    dp_printf(0, 6, FONT_SMALL, 0, "%02d:%02d:%02d%c", hour(t), minute(t),
              second(t), timeState);
#endif // HAS_DCF77 || HAS_IF482
    if (timeSource != _unsynced)
      dp_printf(72, 6, FONT_SMALL, 0, " %2d.%3s%4s", day(t),
                printmonth[month(t)], year(t));

#endif // TIME_SYNC_INTERVAL

#if (HAS_LORA)
    // line 7: update LMiC event display
    dp_printf(0, 7, FONT_SMALL, 0, "%-14s", lmic_event_msg);

    // update LoRa send queue display
    msgWaiting = uxQueueMessagesWaiting(LoraSendQueue);
    if (msgWaiting) {
      sprintf(buff, "%2d", msgWaiting);
      dp_printf(112, 7, FONT_SMALL, 0, "%-2s",
                msgWaiting == SEND_QUEUE_SIZE ? "<>" : buff);
    } else
      dp_printf(112, 7, FONT_SMALL, 0, "  ");
#endif // HAS_LORA

    break; // page0

  case 1:
    dp_printf(0, 4, FONT_STRETCHED, 0, "%02d:%02d:%02d", hour(t), minute(t),
              second(t));
    break; // page1

  case 2:
#if (HAS_GPS)
    if (gps.location.age() < 1500) {
      // line 5: clear "No fix"
      if (wasnofix) {
        dp_printf(16, 5, FONT_STRETCHED, 0, "      ");
        wasnofix = false;
      }
      // line 3-4: GPS latitude
      dp_printf(0, 3, FONT_STRETCHED, 0, "%c%07.4f",
                gps.location.rawLat().negative ? 'S' : 'N', gps.location.lat());

      // line 6-7: GPS longitude
      dp_printf(0, 6, FONT_STRETCHED, 0, "%c%07.4f",
                gps.location.rawLat().negative ? 'W' : 'E', gps.location.lng());

    } else {
      dp_printf(16, 5, FONT_STRETCHED, 1, "No fix");
      wasnofix = true;
    }

#else
    dp_printf(16, 5, FONT_STRETCHED, 1, "No GPS");
#endif

    break; // page2

  case 3:

#if (HAS_BME)
    // line 2-3: Temp
    dp_printf(0, 2, FONT_STRETCHED, 0, "TMP:%-2.1f", bme_status.temperature);

    // line 4-5: Hum
    dp_printf(0, 4, FONT_STRETCHED, 0, "HUM:%-2.1f", bme_status.humidity);

#ifdef HAS_BME680
    // line 6-7: IAQ
    dp_printf(0, 6, FONT_STRETCHED, 0, "IAQ:%-3.0f", bme_status.iaq);
#endif

#else
    dp_printf(16, 5, FONT_STRETCHED, 1, "No BME");
#endif

    break; // page3

  default:
    break; // default

  } // switch

} // draw_page

// display print helper function

void dp_printf(int x, int y, int font, int inv, const char *format, ...) {
  char loc_buf[64];
  char *temp = loc_buf;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if (len < 0) {
    va_end(arg);
    return;
  };
  if (len >= sizeof(loc_buf)) {
    temp = (char *)malloc(len + 1);
    if (temp == NULL) {
      va_end(arg);
      return;
    }
    len = vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
  oledWriteString(0, x, y, temp, font, inv, 1);
  if (temp != loc_buf) {
    free(temp);
  }
}

#endif // HAS_DISPLAY