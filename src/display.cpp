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
3|WIFI:abcde BLTH:abcde   SMALL
4|B:a.bcV Sats:ab ch:ab   SMALL
5|RLIM:abcd  Mem:abcdKB   SMALL
6|27.Feb 2019 20:27:00*   SMALL
7|yyyyyyyyyyyyy xx SFab   SMALL

* = char {L|G|R|?} indicates time source,
    inverse = clock controller is active,
    pulsed = pps input signal is active

y = LMIC event message
xx = payload sendqueue length
ab = LMIC spread factor

MY_FONT_SMALL:     6x8px = 21 chars / line
MY_FONT_NORMAL:    8x8px = 16 chars / line
MY_FONT_STRETCHED: 16x32px = 8 chars / line

*/

// Basic Config
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes
#include "globals.h"
#include "display.h"

// local Tag for logging
static const char TAG[] = __FILE__;

// helper array for converting month values to text
const char *printmonth[] = {"xxx", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
uint8_t DisplayIsOn = 0;
uint8_t displaybuf[MY_DISPLAY_WIDTH * MY_DISPLAY_HEIGHT / 8] = {0};
static uint8_t plotbuf[MY_DISPLAY_WIDTH * MY_DISPLAY_HEIGHT / 8] = {0};
static int dp_row = 0, dp_col = 0, dp_font = 0;

QRCode qrcode;

#ifdef HAS_DISPLAY
#if (HAS_DISPLAY) == 1
OBDISP ssoled;
#elif (HAS_DISPLAY) == 2
TFT_eSPI tft = TFT_eSPI();
#else
#error Unknown display type specified in hal file
#endif
#endif

void dp_setup(int contrast) {

#if (HAS_DISPLAY) == 1 // I2C OLED

  int rc = obdI2CInit(&ssoled, OLED_TYPE, OLED_ADDR, MY_DISPLAY_FLIP,
                      MY_DISPLAY_INVERT, USE_HW_I2C, MY_DISPLAY_SDA,
                      MY_DISPLAY_SCL, MY_DISPLAY_RST,
                      OLED_FREQUENCY); // use standard I2C bus at 400Khz
  assert(rc != OLED_NOT_FOUND);

  // set display buffer
  obdSetBackBuffer(&ssoled, displaybuf);
  obdSetTextWrap(&ssoled, true);
  dp_font = MY_FONT_NORMAL;

#elif (HAS_DISPLAY) == 2 // SPI TFT

  tft.init();
  tft.setRotation(MY_DISPLAY_FLIP ? 3 : 1);
  tft.invertDisplay(MY_DISPLAY_INVERT ? true : false);
  tft.setTextColor(MY_DISPLAY_FGCOLOR, MY_DISPLAY_BGCOLOR);

#endif

  // clear display
  dp_clear();
  if (contrast)
    dp_contrast(contrast);
}

void dp_init(bool verbose) {

#if (HAS_DISPLAY) == 1 // i2c
  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {
#endif

    dp_setup(DISPLAYCONTRAST);

    if (verbose) {

      // show startup screen
      // to come -> display .bmp file with logo

// show chip information
#if (VERBOSE)
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);

      dp_setFont(MY_FONT_NORMAL);
      dp_printf("** PAXCOUNTER **");
      dp_println();
      dp_printf("Software v%s", PROGVERSION);
      dp_println();
      dp_printf("ESP32 %d cores", chip_info.cores);
      dp_println();
      dp_printf("Chip Rev.%d", chip_info.revision);
      dp_println();
      dp_printf("WiFi%s%s", (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
      dp_println();
      dp_printf("%dMB %s Flash", spi_flash_get_chip_size() / (1024 * 1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int."
                                                              : "ext.");

      // give user some time to read or take picture
      dp_dump(displaybuf);
      delay(2000);
      dp_clear();
#endif // VERBOSE

#if (HAS_LORA)
      // generate DEVEUI as QR code and text
      uint8_t buf[8], *p = buf;
      char deveui[17];
      os_getDevEui((u1_t *)buf);
      snprintf(deveui, 17, "%016llX", (*(uint64_t *)(p)));

      // display DEVEUI as QR code on the left
      dp_contrast(30);
      dp_printqr(3, 3, deveui);

      // display DEVEUI as plain text on the right
      const int x_offset = QR_SCALEFACTOR * 29 + 14;
      dp_setTextCursor(x_offset, 0);
      dp_setFont(MY_FONT_NORMAL);
      dp_printf("DEVEUI");
      dp_println();
      for (uint8_t i = 0; i <= 3; i++) {
        dp_setTextCursor(x_offset, i + 3);
        dp_printf("%4.4s", deveui + i * 4);
      }

      // give user some time to read or take picture
      dp_dump(displaybuf);
      delay(8000);
      dp_contrast(DISPLAYCONTRAST);
      dp_clear();
#endif // HAS_LORA

    } // verbose

    dp_power(cfg.screenon); // set display off if disabled

#if (HAS_DISPLAY) == 1  // i2c
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }                     // mutex
#endif

} // dp_init

void dp_refresh(bool nextPage) {

#ifndef HAS_BUTTON
  static uint32_t framecounter = 0;
#endif

  // update histogram
  dp_plotCurve(macs.size(), false);

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
      dp_power(cfg.screenon);
    }

#ifndef HAS_BUTTON
    // auto flip page if we are in unattended mode
    if ((++framecounter) > (DISPLAYCYCLE * 1000 / DISPLAYREFRESH_MS)) {
      framecounter = 0;
      nextPage = true;
    }
#endif

    dp_drawPage(t, nextPage);
    dp_dump(displaybuf);

    I2C_MUTEX_UNLOCK(); // release i2c bus access

  } // mutex
} // refreshDisplay()

void dp_drawPage(time_t t, bool nextpage) {

  // write display content to display buffer
  // nextpage = true -> flip 1 page

  static uint8_t DisplayPage = 0;
  char timeState;
#if (HAS_GPS)
  static bool wasnofix = true;
#endif

  if (nextpage) {
    DisplayPage = (DisplayPage >= DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
    dp_clear();
  }

  // cursor home
  dp_setTextCursor(0, 0);

  // line 1/2: pax counter
  // display number of unique macs total Wifi + BLE
  if (DisplayPage < 5) {
    dp_setFont(MY_FONT_STRETCHED);
    dp_printf("PAX:%-4d", macs.size());
  }

  switch (DisplayPage) {

    // page 0: parameters overview
    // page 1: lorawan parameters
    // page 2: GPS
    // page 3: BME280/680
    // page 4: time
    // page 5: pax graph
    // page 6: blank screen

    // ---------- page 0: parameters overview ----------
  case 0:

    dp_setTextCursor(0, 3);
    dp_setFont(MY_FONT_SMALL);

    // line 3: wifi + bluetooth counters
    // WIFI:abcde BLTH:abcde

#if ((WIFICOUNTER) && (BLECOUNTER))
    if (cfg.wifiscan)
      dp_printf("WIFI:%-5d", macs_wifi);
    else
      dp_printf("WIFI:off");
    if (cfg.blescan)
#if (!COUNT_ENS)
      dp_printf("BLTH:%-5d", macs_ble);
#else
      dp_printf(" CWA:%-5d", cwa_report());
#endif
    else
      dp_printf(" BLTH:off");
#elif ((WIFICOUNTER) && (!BLECOUNTER))
    if (cfg.wifiscan)
      dp_printf("WIFI:%-5d", macs_wifi);
    else
      dp_printf("WIFI:off");
#elif ((!WIFICOUNTER) && (BLECOUNTER))
    if (cfg.blescan) {
      dp_printf("BLTH:%-5d", macs_ble);
#if (COUNT_ENS)
      dp_printf("(CWA:%d)", cwa_report());
#endif
    } else
      dp_printf("BLTH:off");
#else
    dp_printf("Sniffer disabled");
#endif
    dp_println();

    // line 4: Battery + GPS status + Wifi channel
    // B:a.bcV Sats:ab ch:ab
#if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
    if (batt_level == 0)
      dp_printf("No batt ");
    else
      dp_printf("B:%3d%%  ", batt_level);
#else
    dp_printf("       ");
#endif

#if (HAS_GPS)
    dp_setFont(MY_FONT_SMALL, !gps_hasfix());
    dp_printf("Sats:%.2d", gps.satellites.value());
    dp_setFont(MY_FONT_SMALL);
#else
    dp_printf("       ");
#endif
    dp_printf(" ch:%02d", channel);
    dp_println();

    // line 5: RSSI limiter + free memory
    // RLIM:abcd  Mem:abcdKB
    dp_printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d", cfg.rssilimit);
    dp_printf("  Mem:%4dKB", getFreeRAM() / 1024);
    dp_println();

    // line 6: time + date
    // 27.Feb 2019 20:27:00*
#if (TIME_SYNC_INTERVAL)
    timeState = TimePulseTick ? ' ' : timeSetSymbols[timeSource];
    TimePulseTick = false;

    dp_printf("%02d.%3s %4d", day(t), printmonth[month(t)], year(t));
    dp_printf(" %02d:%02d:%02d", hour(t), minute(t), second(t));

// display inverse timeState if clock controller is enabled
#if (defined HAS_DCF77) || (defined HAS_IF482)
    dp_setFont(MY_FONT_SMALL, 1);
    dp_printf("%c", timeState);
    dp_setFont(MY_FONT_SMALL, 0);
#else
    dp_printf("%c", timeState);
#endif
    dp_println();
#endif // TIME_SYNC_INTERVAL

    // line 7: LORA network status
    // yyyyyyyyyyyyy xx SFab

#if (HAS_LORA)
    // LMiC event display
    dp_printf("%-16s", lmic_event_msg);
    // LORA datarate, display inverse if ADR disabled
    dp_setFont(MY_FONT_SMALL, !cfg.adrmode);
    dp_printf(" %-4s", getSfName(updr2rps(LMIC.datarate)));
    dp_setFont(MY_FONT_SMALL, 0);
    dp_println();
#endif // HAS_LORA
    break;

  // ---------- page 1: lorawan parameters ----------
  case 1:

#if (HAS_LORA)

    // 3|Net:000000   Pwr:aa
    // 4|Dev:00000000 DR:0
    // 5|CHMsk:0000 Nonce:0000
    // 6|fUp:000000 fDn:000000
    // 7|SNR:-0000  RSSI:-0000

    dp_setFont(MY_FONT_SMALL);
    dp_setTextCursor(0, 3);
    dp_printf("Net:%06X   Pwr:%-2d", LMIC.netid & 0x001FFFFF, LMIC.radio_txpow);
    dp_println();
    dp_printf("Dev:%08X DR:%1d", LMIC.devaddr, LMIC.datarate);
    dp_println();
    dp_printf("ChMsk:%04X Nonce:%04X", LMIC.channelMap, LMIC.devNonce);
    dp_println();
    dp_printf("fUp:%-6d fDn:%-6d", LMIC.seqnoUp ? LMIC.seqnoUp - 1 : 0,
              LMIC.seqnoDn ? LMIC.seqnoDn - 1 : 0);
    dp_println();
    dp_printf("SNR:%-5d  RSSI:%-5d", (LMIC.snr + 2) / 4, LMIC.rssi);
    break;
#else  // flip page if we are unattended
    DisplayPage++;
#endif // HAS_LORA

  // ---------- page 2: GPS ----------
  case 2:

#if (HAS_GPS)
    dp_setFont(MY_FONT_STRETCHED);
    dp_setTextCursor(0, 3);
    if (gps_hasfix()) {
      // line 5: clear "No fix"
      if (wasnofix) {
        dp_setTextCursor(2, 4);
        dp_printf("      ");
        wasnofix = false;
      }
      // line 3-4: GPS latitude
      dp_printf("%c%07.4f", gps.location.rawLat().negative ? 'S' : 'N',
                gps.location.lat());

      // line 6-7: GPS longitude
      dp_printf("%c%07.4f", gps.location.rawLat().negative ? 'W' : 'E',
                gps.location.lng());

    } else {
      dp_setTextCursor(2, 4);
      dp_setFont(MY_FONT_STRETCHED, 1);
      dp_printf("No fix");
      wasnofix = true;
    }
    break;
#else // flip page if we are unattended
    DisplayPage++;
#endif

  // ---------- page 3: BME280/680 ----------
  case 3:

#if (HAS_BME)
    dp_setFont(MY_FONT_STRETCHED);
    dp_setTextCursor(0, 2);

    // line 2-3: Temp
    dp_printf("TMP:%-2.1f", bme_status.temperature);
    dp_println(2);

    // line 4-5: Hum
    dp_printf("HUM:%-2.1f", bme_status.humidity);
    dp_println(2);

#ifdef HAS_BME680
    // line 6-7: IAQ
    dp_printf("IAQ:%-3.0f", bme_status.iaq);
#else      // is BME280 or BMP180
    // line 6-7: Pre
    dp_printf("PRE:%-2.1f", bme_status.pressure);
#endif     // HAS_BME680
    break; // page 3
#else      // flip page if we are unattended
    DisplayPage++;
#endif     // HAS_BME

  // ---------- page 4: time ----------
  case 4:

    dp_setFont(MY_FONT_LARGE);
    dp_setTextCursor(0, 4);
    dp_printf("%02d:%02d:%02d", hour(t), minute(t), second(t));
    break;

  // ---------- page 5: pax graph ----------
  case 5:

    dp_setFont(MY_FONT_NORMAL);
    dp_setTextCursor(0, 0);
    dp_printf("Pax graph");
    dp_dump(plotbuf);
    break;

  // ---------- page 6: blank screen ----------
  case 6:

#ifdef HAS_BUTTON
    dp_clear();
    break;
#else // flip page if we are unattended
    DisplayPage++;
#endif

  } // switch (page)
} // dp_drawPage

// ------------- display helper functions -----------------

void dp_setTextCursor(int x, int y) {
  // x represents the pixel column
  // y represents the text row
  dp_col = x;

#if (HAS_DISPLAY) == 1
  dp_row = y;
  obdSetCursor(&ssoled, dp_col, dp_row);

#elif (HAS_DISPLAY) == 2
  switch (dp_font >> 1) {
  case MY_FONT_STRETCHED:
  case MY_FONT_LARGE:
    dp_row = y * 26;
    break;
  case MY_FONT_SMALL:
  case MY_FONT_NORMAL:
  default:
    dp_row = y * 16;
    break;
  }
  tft.setCursor(dp_col, dp_row);
#endif
}

void dp_setFont(int font, int inv) {
#if (HAS_DISPLAY) == 1
  dp_font = (font << 1) | (inv & 0x01);
#elif (HAS_DISPLAY) == 2
  // map font oled -> tft
  switch (font) {
  case MY_FONT_STRETCHED: // 16x16 on OLED
  case MY_FONT_LARGE:     // 16x32 on OLED
    tft.setTextFont(4);   // 26px
    break;
  case MY_FONT_SMALL:  // 6x8 on OLED
  case MY_FONT_NORMAL: // 8x8 on OLED
  default:
    tft.setTextFont(2); // 16px
    break;
  }
  // to do: invers printing
#endif
}

void dp_println(int lines) {
  dp_col = 0;
  dp_row += lines;
#if (HAS_DISPLAY) == 1
  dp_setTextCursor(dp_col, dp_row);
#elif (HAS_DISPLAY) == 2
  for (int i = 1; i <= lines; i++)
    tft.println();
#endif
};

void dp_printf(const char *format, ...) {
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
    vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
#if (HAS_DISPLAY) == 1
  obdWriteString(&ssoled, 0, -1, dp_row, temp, dp_font >> 1, dp_font & 0x01,
                 false);
#elif (HAS_DISPLAY) == 2
  tft.printf(temp);
#endif
  if (temp != loc_buf) {
    free(temp);
  }
}

void dp_dump(uint8_t *pBuffer) {
#if (HAS_DISPLAY) == 1
  obdDumpBuffer(&ssoled, pBuffer);
#elif (HAS_DISPLAY) == 2
  // probably oled buffer stucture is not suitable for tft -> to be checked
  tft.drawBitmap(0, 0, pBuffer, MY_DISPLAY_WIDTH, MY_DISPLAY_HEIGHT,
                 MY_DISPLAY_FGCOLOR);
#endif
}

void dp_clear(void) {
  dp_setTextCursor(0, 0);
#if (HAS_DISPLAY) == 1
  obdFill(&ssoled, 0, 1);
#elif (HAS_DISPLAY) == 2
  tft.fillScreen(MY_DISPLAY_BGCOLOR);
#endif
}

void dp_contrast(uint8_t contrast) {
#if (HAS_DISPLAY) == 1
  obdSetContrast(&ssoled, contrast);
#elif (HAS_DISPLAY) == 2
  // to do: gamma correction for TFT
#endif
}

void dp_power(uint8_t screenon) {
#if (HAS_DISPLAY) == 1
  obdPower(&ssoled, screenon);
#elif (HAS_DISPLAY) == 2
  // to come
#endif
}

void dp_shutdown(void) {
#if (HAS_DISPLAY) == 1
  // block i2c bus access
  if (!I2C_MUTEX_LOCK())
    ESP_LOGV(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  else {
    cfg.screenon = 0;
    obdPower(&ssoled, false);
    delay(DISPLAYREFRESH_MS / 1000 * 1.1);
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }
#elif (HAS_DISPLAY) == 2
  // to come
#endif
}

// ------------- QR code plotter -----------------

void dp_printqr(uint16_t offset_x, uint16_t offset_y, const char *Message) {
  uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
  qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_HIGH, Message);

  // draw QR code
  for (uint8_t y = 0; y < qrcode.size; y++)
    for (uint8_t x = 0; x < qrcode.size; x++)
      if (!qrcode_getModule(&qrcode, x, y)) // "black"
        dp_fillRect(x * QR_SCALEFACTOR + offset_x,
                    y * QR_SCALEFACTOR + offset_y, QR_SCALEFACTOR,
                    QR_SCALEFACTOR, false);
  // draw horizontal frame lines
  dp_fillRect(0, 0, qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y,
              false);
  dp_fillRect(0, qrcode.size * QR_SCALEFACTOR + offset_y,
              qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y, false);
  // draw vertical frame lines
  dp_fillRect(0, 0, offset_x, qrcode.size * QR_SCALEFACTOR + 2 * offset_y,
              false);
  dp_fillRect(qrcode.size * QR_SCALEFACTOR + offset_x, 0, offset_x,
              qrcode.size * QR_SCALEFACTOR + 2 * offset_y, false);
}

// ------------- graphics primitives -----------------

void dp_fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 uint8_t bRender) {
#if (HAS_DISPLAY) == 1
  for (uint16_t xi = x; xi < x + width; xi++)
    obdDrawLine(&ssoled, xi, y, xi, y + height - 1, 1, bRender);
#elif (HAS_DISPLAY) == 2
  tft.fillRect(x, y, width, height, MY_DISPLAY_FGCOLOR);
#endif
}

int dp_drawPixel(uint8_t *buf, const uint16_t x, const uint16_t y,
                 const uint8_t dot) {

  if (x > MY_DISPLAY_WIDTH || y > MY_DISPLAY_HEIGHT)
    return -1;

  uint8_t bit = y & 7;
  uint16_t idx = y / 8 * MY_DISPLAY_WIDTH + x;

  buf[idx] &= ~(1 << bit); // clear pixel
  if (dot)
    buf[idx] |= (1 << bit); // set pixel

  return 0;
}

// ------------- buffer scroll functions -----------------

void dp_scrollHorizontal(uint8_t *buf, const uint16_t width,
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

void dp_scrollVertical(uint8_t *buf, const uint16_t width,
                       const uint16_t height, int offset) {

  uint64_t buf_col;

  if (!offset)
    return; // nothing to do

  for (uint16_t col = 0; col < MY_DISPLAY_WIDTH; col++) {
    // convert column bytes from display buffer to uint64_t
    buf_col = *(uint64_t *)&buf[col * MY_DISPLAY_HEIGHT / 8];

    if (offset > 0) // scroll down
      buf_col <<= offset;
    else // scroll up
      buf_col >>= abs(offset);

    // write back uint64_t to uint8_t display buffer
    *(uint64_t *)&buf[col * MY_DISPLAY_HEIGHT / 8] = buf_col;
  }
}

// ------------- curve plotter -----------------

void dp_plotCurve(uint16_t count, bool reset) {

  static uint16_t last_count = 0, col = 0, row = 0;
  uint16_t v_scroll = 0;

  if ((last_count == count) && !reset)
    return;

  if (reset) {                      // next count cycle?
    if (col < MY_DISPLAY_WIDTH - 1) // matrix not full -> increment column
      col++;
    else // matrix full -> scroll left 1 dot
      dp_scrollHorizontal(plotbuf, MY_DISPLAY_WIDTH, MY_DISPLAY_HEIGHT, true);

  } else // clear current dot
    dp_drawPixel(plotbuf, col, row, 0);

  // scroll down, if necessary
  while ((count - v_scroll) > MY_DISPLAY_HEIGHT - 1)
    v_scroll++;
  if (v_scroll)
    dp_scrollVertical(plotbuf, MY_DISPLAY_WIDTH, MY_DISPLAY_HEIGHT, v_scroll);

  // set new dot
  // row = MY_DISPLAY_HEIGHT - 1 - (count - v_scroll) % MY_DISPLAY_HEIGHT;
  row = MY_DISPLAY_HEIGHT - 1 - count - v_scroll;
  last_count = count;
  dp_drawPixel(plotbuf, col, row, 1);
}

#endif // HAS_DISPLAY
