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
7|yyyyyyyyyyyyy xx SFab   SMALL

* = char {L|G|R|?} indicates time source,
    inverse = clock controller is active,
    pulsed = pps input signal is active

y = LMIC event message
xx = payload sendqueue length
ab = LMIC spread factor

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

#define DISPLAY_PAGES (6) // number of paxcounter display pages

// settings for oled display library
#define USE_BACKBUFFER

// settings for qr code generator
#define QR_VERSION 3     // 29 x 29px
#define QR_SCALEFACTOR 2 // 29 -> 58x < 64px

// settings for curve plotter
#define DISPLAY_WIDTH 128 // Width in pixels of OLED-display, must be 32X
#define DISPLAY_HEIGHT 64 // Height in pixels of OLED-display, must be 64X

// helper array for converting month values to text
const char *printmonth[] = {"xxx", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
uint8_t DisplayIsOn = 0;
uint8_t displaybuf[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8] = {0};
static uint8_t plotbuf[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8] = {0};

QRCode qrcode;

void init_display(bool verbose) {

  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {

    // is we have display RST line we toggle it to re-initialize display
#ifdef MY_OLED_RST
    pinMode(MY_OLED_RST, OUTPUT);
    digitalWrite(MY_OLED_RST, 0); // initialization of SSD1306 chip is executed
    delay(1); // keep RES low for at least 3us according to SSD1306 datasheet
    digitalWrite(MY_OLED_RST, 1); // normal operation
#endif

    // init display
#ifndef DISPLAY_FLIP
    oledInit(OLED_128x64, false, false, -1, -1, 400000L);
#else
    oledInit(OLED_128x64, true, false, -1, -1, 400000L);
#endif

    // set display buffer
    oledSetBackBuffer(displaybuf);

    // clear display
    oledSetContrast(DISPLAYCONTRAST);
    oledFill(0, 1);

    if (verbose) {

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
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int."
                                                              : "ext.");

      // give user some time to read or take picture
      oledDumpBuffer(displaybuf);
      delay(2000);
      oledFill(0x00, 1);
#endif // VERBOSE

#if (HAS_LORA)
      // generate DEVEUI as QR code and text
      uint8_t buf[8];
      char deveui[17];
      os_getDevEui((u1_t *)buf);
      snprintf(deveui, 17, "%016llX", *((uint64_t *)&buf));

      // display DEVEUI as QR code on the left
      oledSetContrast(30);
      dp_printqr(3, 3, deveui);

      // display DEVEUI as plain text on the right
      dp_printf(72, 0, FONT_NORMAL, 0, "LORAWAN");
      dp_printf(72, 1, FONT_NORMAL, 0, "DEVEUI:");
      for (uint8_t i = 0; i <= 3; i++)
        dp_printf(80, i + 3, FONT_NORMAL, 0, "%4.4s", deveui + i * 4);

      // give user some time to read or take picture
      oledDumpBuffer(displaybuf);
      delay(8000);
      oledSetContrast(DISPLAYCONTRAST);
      oledFill(0x00, 1);
#endif // HAS_LORA

    } // verbose

    oledPower(cfg.screenon); // set display off if disabled

    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }                     // mutex
} // init_display

void refreshTheDisplay(bool nextPage) {

#ifndef HAS_BUTTON
  static uint32_t framecounter = 0;
#endif

  // update histogram
  oledPlotCurve(macs.size(), false);

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

#ifndef HAS_BUTTON
    // auto flip page if we are in unattended mode
    if ((++framecounter) > (DISPLAYCYCLE * 1000 / DISPLAYREFRESH_MS)) {
      framecounter = 0;
      nextPage = true;
    }
#endif

    draw_page(t, nextPage);
    oledDumpBuffer(displaybuf);

    I2C_MUTEX_UNLOCK(); // release i2c bus access

  } // mutex
} // refreshDisplay()

void shutdown_display(void) {
  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {
    cfg.screenon = 0;
    oledShutdown();
    delay(DISPLAYREFRESH_MS / 1000 * 1.1);
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }
}

void draw_page(time_t t, bool nextpage) {

  // write display content to display buffer
  // nextpage = true -> flip 1 page

  static uint8_t DisplayPage = 0;
  char timeState;
#if (HAS_GPS)
  static bool wasnofix = true;
#endif

  // line 1/2: pax counter
  dp_printf(0, 0, FONT_STRETCHED, 0, "PAX:%-4d",
            macs.size()); // display number of unique macs total Wifi + BLE

start:

  if (nextpage) {
    DisplayPage = (DisplayPage >= DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
    oledFill(0, 1);
  }

  switch (DisplayPage) {

    // page 0: parameters overview
    // page 1: pax graph
    // page 2: GPS
    // page 3: BME280/680
    // page 4: time
    // page 5: blank screen

    // page 0: parameters overview
  case 0:

    // line 3: wifi + bluetooth counters
#if ((WIFICOUNTER) && (BLECOUNTER))
    if (cfg.wifiscan)
      dp_printf(0, 3, FONT_SMALL, 0, "WIFI:%-5d", macs_wifi);
    else
      dp_printf(0, 3, FONT_SMALL, 0, "%s", "WIFI:off");
    if (cfg.blescan)
      dp_printf(66, 3, FONT_SMALL, 0, "BLTH:%-5d", macs_ble);
    else
      dp_printf(66, 3, FONT_SMALL, 0, "%s", "BLTH:off");
#elif ((WIFICOUNTER) && (!BLECOUNTER))
    if (cfg.wifiscan)
      dp_printf(0, 3, FONT_SMALL, 0, "WIFI:%-5d", macs_wifi);
    else
      dp_printf(0, 3, FONT_SMALL, 0, "%s", "WIFI:off");
#elif ((!WIFICOUNTER) && (BLECOUNTER))
    if (cfg.blescan)
      dp_printf(0, 3, FONT_SMALL, 0, "BLTH:%-5d", macs_ble);
    else
      dp_printf(0, 3, FONT_SMALL, 0, "%s", "BLTH:off");
#else
    dp_printf(0, 3, FONT_SMALL, 0, "%s", "Sniffer disabled");
#endif

// line 4: Battery + GPS status + Wifi channel
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
    if (batt_voltage == 0xffff)
      dp_printf(0, 4, FONT_SMALL, 0, "%s", "USB    ");
    else if (batt_voltage == 0)
      dp_printf(0, 4, FONT_SMALL, 0, "%s", "No batt");
    else
      dp_printf(0, 4, FONT_SMALL, 0, "B:%.2fV", batt_voltage / 1000.0);
#endif
#if (HAS_GPS)
    if (gps_hasfix())
      dp_printf(48, 4, FONT_SMALL, 0, "Sats:%.2d", gps.satellites.value());
    else // if no fix then display Sats value inverse
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
    // LMiC event display
    dp_printf(0, 7, FONT_SMALL, 0, "%-16s", lmic_event_msg);
    // LORA datarate, display inverse if ADR disabled
    dp_printf(102, 7, FONT_SMALL, !cfg.adrmode, "%-4s",
              getSfName(updr2rps(LMIC.datarate)));
#endif     // HAS_LORA
    break; // page0

    // page 1: pax graph
  case 1:
    oledDumpBuffer(plotbuf);
    break; // page1

    // page 2: GPS
  case 2:
#if (HAS_GPS)
    if (gps_hasfix()) {
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
    break; // page2
#else
    DisplayPage++; // next page
#endif

    // page 3: BME280/680
  case 3:
#if (HAS_BME)
    // line 2-3: Temp
    dp_printf(0, 2, FONT_STRETCHED, 0, "TMP:%-2.1f", bme_status.temperature);

    // line 4-5: Hum
    dp_printf(0, 4, FONT_STRETCHED, 0, "HUM:%-2.1f", bme_status.humidity);

#ifdef HAS_BME680
    // line 6-7: IAQ
    dp_printf(0, 6, FONT_STRETCHED, 0, "IAQ:%-3.0f", bme_status.iaq);
#else      // is BME280 or BMP180
    // line 6-7: Pre
    dp_printf(0, 6, FONT_STRETCHED, 0, "PRE:%-2.1f", bme_status.pressure);
#endif     // HAS_BME680
    break; // page 3
#else
    DisplayPage++; // next page
#endif // HAS_BME

  // page 4: time
  case 4:
    dp_printf(0, 4, FONT_LARGE, 0, "%02d:%02d:%02d", hour(t), minute(t),
              second(t));
    break;

    // page 5: blank screen
  case 5:
#ifdef HAS_BUTTON
    oledFill(0, 1);
    break;
#else // don't show blank page if we are unattended
    DisplayPage++; // next page
#endif

  default:
    goto start; // start over

  } // switch

} // draw_page

// display helper functions
void dp_printf(uint16_t x, uint16_t y, uint8_t font, uint8_t inv,
               const char *format, ...) {
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

void dp_printqr(uint16_t offset_x, uint16_t offset_y, const char *Message) {
  uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
  qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_HIGH, Message);

  // draw QR code
  for (uint16_t y = 0; y < qrcode.size; y++)
    for (uint16_t x = 0; x < qrcode.size; x++)
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

void oledfillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uint8_t bRender) {
  for (uint16_t xi = x; xi < x + width; xi++)
    oledDrawLine(xi, y, xi, y + height - 1, bRender);
}

int oledDrawPixel(uint8_t *buf, const uint16_t x, const uint16_t y,
                  const uint8_t dot) {

  if (x > DISPLAY_WIDTH || y > DISPLAY_HEIGHT)
    return -1;

  uint8_t bit = y & 7;
  uint16_t idx = y / 8 * DISPLAY_WIDTH + x;

  buf[idx] &= ~(1 << bit); // clear pixel
  if (dot)
    buf[idx] |= (1 << bit); // set pixel

  return 0;
}

void oledScrollBufferHorizontal(uint8_t *buf, const uint16_t width,
                                const uint16_t height, bool left) {

  uint16_t col, page, idx = 0;

  for (page = 0; page < height / 8; page++) {
    if (left) { // scroll left
      for (col = 0; col < width - 1; col++) {
        idx = page * width + col;
        buf[idx] = buf[idx + 1];
      }
      buf[idx + 1] = 0;
    } else // scroll right
    {
      for (col = width - 1; col > 0; col--) {
        idx = page * width + col;
        buf[idx] = buf[idx - 1];
      }
      buf[idx - 1] = 0;
    }
  }
}

void oledScrollBufferVertical(uint8_t *buf, const uint16_t width,
                              const uint16_t height, int offset) {

  uint64_t buf_col;

  if (!offset)
    return; // nothing to do

  for (uint16_t col = 0; col < DISPLAY_WIDTH; col++) {
    // convert column bytes from display buffer to uint64_t
    buf_col = *(uint64_t *)&buf[col * DISPLAY_HEIGHT / 8];

    if (offset > 0) // scroll down
      buf_col <<= offset;
    else // scroll up
      buf_col >>= abs(offset);

    // write back uint64_t to uint8_t display buffer
    *(uint64_t *)&buf[col * DISPLAY_HEIGHT / 8] = buf_col;
  }
}

void oledPlotCurve(uint16_t count, bool reset) {

  static uint16_t last_count = 0, col = 0, row = 0;
  uint16_t v_scroll = 0;

  if ((last_count == count) && !reset)
    return;

  if (reset) {                   // next count cycle?
    if (col < DISPLAY_WIDTH - 1) // matrix not full -> increment column
      col++;
    else // matrix full -> scroll left 1 dot
      oledScrollBufferHorizontal(plotbuf, DISPLAY_WIDTH, DISPLAY_HEIGHT, true);

  } else // clear current dot
    oledDrawPixel(plotbuf, col, row, 0);

  // scroll down, if necessary
  while ((count - v_scroll) > DISPLAY_HEIGHT - 1)
    v_scroll++;
  if (v_scroll)
    oledScrollBufferVertical(plotbuf, DISPLAY_WIDTH, DISPLAY_HEIGHT, v_scroll);

  // set new dot
  // row = DISPLAY_HEIGHT - 1 - (count - v_scroll) % DISPLAY_HEIGHT;
  row = DISPLAY_HEIGHT - 1 - count - v_scroll;
  last_count = count;
  oledDrawPixel(plotbuf, col, row, 1);
}

#endif // HAS_DISPLAY