#ifdef HAS_DISPLAY

/*

Display-Mask (128 x 64 pixel):

 |        |       |
 |          11111111112
 |012345678901234567890   Font
-----------------------   ---------
0|PAX:aabbccdd      	    STRETCHED
1|PAX:aabbccdd            STRETCHED
2|
3|B:a.bcV Sats:ab ch:ab   SMALL
4|WIFI:abcde BLTH:abcde   SMALL
5|RLIM:abcd  Mem:abcdKB   SMALL
6|27.Feb 2019 20:27:00*   SMALL
7|yyyyyyyyyyyyyyyy SFab   SMALL

* = char {L|G|R|?} indicates time source,
    inverse = clock controller is active,
    pulsed = pps input signal is active

y = LMIC event message; ab = payload queue length

FONT_SMALL:     6x8px = 21 chars / line
FONT_NORMAL:    8x8px = 16 chars / line
FONT_STRETCHED: 16x32px = 8 chars / line

*/

// Basic Config
#include "globals.h"
#include <ss_oled.h>
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

// local Tag for logging
static const char TAG[] = __FILE__;

// settings for oled display library
#define DISPLAY_PAGES (4) // number of display pages
#define USE_BACKBUFFER    // for display library

// settings for qr code generator
#define QR_VERSION 3     // 29 x 29px
#define QR_SCALEFACTOR 2 // 29 -> 58x < 64px

// helper arry for converting month values to text
const char *printmonth[] = {"xxx", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

uint8_t DisplayIsOn = 0;

QRCode qrcode;

void init_display(void) {

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
    oledSetContrast(DISPLAYCONTRAST);
    oledFill(0, 1);

    // show startup screen
    // to come -> display .bmp file with logo

// show chip information
#if (VERBOSE)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    dp_printf(0, 0, 0, 0, "** PAXCOUNTER **");
    dp_printf(0, 1, 0, 0, "Software v%s", PROGVERSION);
    dp_printf(0, 3, 0, 0, "ESP32 %d cores", chip_info.cores);
    dp_printf(0, 4, 0, 0, "Chip Rev.%d", chip_info.revision);
    dp_printf(0, 5, 0, 0, "WiFi%s%s",
              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    dp_printf(0, 6, 0, 0, "%dMB %s Flash",
              spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");

    // give user some time to read or take picture
    oledDumpBuffer(NULL);
    delay(2000);
    oledFill(0x00, 1);
#endif // VERBOSE

#if (HAS_LORA)
    // generate DEVEUI as QR code and text
    uint8_t buf[8];
    char deveui[17];
    os_getDevEui((u1_t *)buf);
    sprintf(deveui, "%016llX", *((uint64_t *)&buf));

    // display DEVEUI as QR code on the left
    oledSetContrast(30);
    dp_printqr(3, 3, deveui);

    // display DEVEUI as plain text on the right
    dp_printf(72, 0, FONT_NORMAL, 0, "LORAWAN");
    dp_printf(72, 1, FONT_NORMAL, 0, "DEVEUI:");
    dp_printf(80, 3, FONT_NORMAL, 0, "%4.4s", deveui);
    dp_printf(80, 4, FONT_NORMAL, 0, "%4.4s", deveui + 4);
    dp_printf(80, 5, FONT_NORMAL, 0, "%4.4s", deveui + 8);
    dp_printf(80, 6, FONT_NORMAL, 0, "%4.4s", deveui + 12);

    // give user some time to read or take picture
    oledDumpBuffer(NULL);
    delay(8000);
    oledSetContrast(DISPLAYCONTRAST);
    oledFill(0x00, 1);
#endif // HAS_LORA

    oledPower(cfg.screenon); // set display off if disabled

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
    oledDumpBuffer(NULL);

    I2C_MUTEX_UNLOCK(); // release i2c bus access

  } // mutex
} // refreshDisplay()

void draw_page(time_t t, uint8_t page) {

  char timeState;
  uint8_t msgWaiting;
#if (HAS_GPS)
  static bool wasnofix = true;
#endif

  // line 1/2: pax counter
  dp_printf(0, 0, FONT_STRETCHED, 0, "PAX:%-4d",
            (int)macs.size()); // display number of unique macs total Wifi + BLE

  switch (page % DISPLAY_PAGES) {

    // page 0: parameters overview
    // page 1: time
    // page 2: GPS
    // page 3: BME280/680

  case 0:

    // line 3: wifi + bluetooth counters
    dp_printf(0, 3, FONT_SMALL, 0, "WIFI:%-5d", macs_wifi);
#if (BLECOUNTER)
    if (cfg.blescan)
      dp_printf(66, 3, FONT_SMALL, 0, "BLTH:%-5d", macs_ble);
    else
      dp_printf(66, 3, FONT_SMALL, 0, "%s", "BLTH:off");
#endif

// line 4: Battery + GPS status + Wifi channel
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
    if (batt_voltage == 0xffff)
      dp_printf(0, 4, FONT_SMALL, 0, "%s", "B:USB  ");
    else
      dp_printf(0, 4, FONT_SMALL, 0, "B:%.2fV", batt_voltage / 1000.0);
#endif
#if (HAS_GPS)
    if (gps.location.age() < 1500) // if no fix then display Sats value inverse
      dp_printf(48, 4, FONT_SMALL, 0, "Sats:%.2d", gps.satellites.value());
    else
      dp_printf(48, 4, FONT_SMALL, 1, "Sats:%.2d", gps.satellites.value());
#endif
    dp_printf(96, 4, FONT_SMALL, 0, "ch:%02d", channel);

    // line 5: RSSI limiter + free memory
    dp_printf(0, 5, FONT_SMALL, 0, !cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d",
              cfg.rssilimit);
    dp_printf(66, 5, FONT_SMALL, 0, "Mem:%4dKB", getFreeRAM() / 1024);

    // line 6: time + date
#if (TIME_SYNC_INTERVAL)
    timeState = TimePulseTick ? ' ' : timeSetSymbols[timeSource];
    TimePulseTick = false;

    dp_printf(0, 6, FONT_SMALL, 0, "%02d.%3s %4d", day(t), printmonth[month(t)],
              year(t));
    dp_printf(72, 6, FONT_SMALL, 0, "%02d:%02d:%02d", hour(t), minute(t),
              second(t));

// display inverse timeState if clock controller is enabled
#if (defined HAS_DCF77) || (defined HAS_IF482)
    dp_printf(120, 6, FONT_SMALL, 1, "%c", timeState);
#else
    dp_printf(120, 6, FONT_SMALL, 0, "%c", timeState);
#endif

#endif // TIME_SYNC_INTERVAL

    // line 7: LORA network status
#if (HAS_LORA)
    // LMiC event display, display inverse if sendqueue not empty
    msgWaiting = uxQueueMessagesWaiting(LoraSendQueue);
    if (msgWaiting)
      dp_printf(0, 7, FONT_SMALL, 1, "%-17s", lmic_event_msg);
    else
      dp_printf(0, 7, FONT_SMALL, 0, "%-17s", lmic_event_msg);
    // LORA datarate, display inverse if ADR disabled
    if (cfg.adrmode)
      dp_printf(108, 7, FONT_SMALL, 0, "%-4s",
                getSfName(updr2rps(LMIC.datarate)));
    else
      dp_printf(108, 7, FONT_SMALL, 1, "%-4s",
                getSfName(updr2rps(LMIC.datarate)));
#endif // HAS_LORA

    break; // page0

  case 1:
    dp_printf(0, 4, FONT_LARGE, 0, "%02d:%02d:%02d", hour(t), minute(t),
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

// display print helper functions
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
  oledWriteString(0, x, y, temp, font, inv, false);
  if (temp != loc_buf) {
    free(temp);
  }
}

void dp_printqr(int offset_x, int offset_y, const char *Message) {
  uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
  qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_HIGH, Message);

  // draw QR code
  for (int y = 0; y < qrcode.size; y++)
    for (int x = 0; x < qrcode.size; x++)
      if (!qrcode_getModule(&qrcode, x, y)) // "black"
        oledfillRect(x * QR_SCALEFACTOR + offset_x,
                     y * QR_SCALEFACTOR + offset_y, QR_SCALEFACTOR,
                     QR_SCALEFACTOR, false);
  // draw horizontal frame lines
  oledfillRect(0, 0, qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y,
               false);
  oledfillRect(0, qrcode.size * QR_SCALEFACTOR + offset_y,
               qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y, false);
  // draw vertical frame lines
  oledfillRect(0, 0, offset_x, qrcode.size * QR_SCALEFACTOR + 2 * offset_y,
               false);
  oledfillRect(qrcode.size * QR_SCALEFACTOR + offset_x, 0, offset_x,
               qrcode.size * QR_SCALEFACTOR + 2 * offset_y, false);
}

void oledfillRect(int x, int y, int width, int height, int bRender) {
  for (int xi = x; xi < x + width; xi++)
    oledDrawLine(xi, y, xi, y + height - 1, bRender);
}

#endif // HAS_DISPLAY