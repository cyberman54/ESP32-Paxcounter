#ifdef HAS_DISPLAY

/*

Display-Mask (128 x 64 pixel):

 |        |       |
 |          11111111112
 |012345678901234567890   Font
-----------------------   ---------
0|PAX:aabbccdd      	    LARGE
1|PAX:aabbccdd            LARGE
2|
3|WIFI:abcde BLTH:abcde   SMALL
4|Batt:abc% chan:ab       SMALL
5|RLIM:abcd Mem:abcdeKB   SMALL
6|27.Feb 2019 20:27:00*   SMALL
7|yyyyyyyyyyyyy xx SFab   SMALL

* = char {L|G|R|?} indicates time source,
    inverse = clock controller is active,
    pulsed = pps input signal is active

y = LMIC event message
xx = payload sendqueue length
ab = LMIC spread factor

MY_FONT_SMALL:     6x8px = 21 chars / line @ 8 lines
MY_FONT_NORMAL:    8x8px = 16 chars / line @ 8 lines
MY_FONT_STRETCHED: 12x16px = 10 chars / line @ 4 lines
MY_FONT_LARGE:     16x32px = 8 chars / line @ 2 lines

*/

// Basic Config
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes
#include "globals.h"
#include "display.h"

static uint8_t plotbuf[PLOTBUFFERSIZE] = {0};
uint8_t DisplayIsOn = 0;
hw_timer_t *displayIRQ = NULL;
static QRCode qrcode;

// select display driver
#ifdef HAS_DISPLAY
#if (HAS_DISPLAY) == 1
ONE_BIT_DISPLAY *dp = NULL;
#elif (HAS_DISPLAY) > 1
BB_SPI_LCD *dp = NULL;
#else
#error Unknown display type specified in hal file
#endif
#endif

#define DISPLAY_PAGE_PAX_PARAM_OVERVIEW 0
#define DISPLAY_PAGE_PAX_LORAWAN_PARAM  1
#define DISPLAY_PAGE_PAX_GPS_LAT_LONG   2
#define DISPLAY_PAGE_BME280_680_VALUES  3
#define DISPLAY_PAGE_TIME_OF_DAY        4
#define DISPLAY_PAGE_POWER_OVERVIEW     5
#define DISPLAY_PAGE_PAX_GRAPH          6
#define DISPLAY_PAGE_BLANK_SCREEN       7

void dp_setup(int contrast) {
#if (HAS_DISPLAY) == 1 // I2C OLED

  dp = new ONE_BIT_DISPLAY;
  dp->setI2CPins(MY_DISPLAY_SDA, MY_DISPLAY_SCL, MY_DISPLAY_RST);
  dp->setBitBang(false);
  dp->I2Cbegin(OLED_TYPE, OLED_ADDR, OLED_FREQUENCY);
  dp->allocBuffer(); // render all outputs to lib internal backbuffer
  dp->setTextWrap(false);
  dp->setRotation(
      MY_DISPLAY_FLIP ? 2 : 0); // 0 = no rotation, 1 = 90°, 2 = 180°, 3 = 270°

#elif (HAS_DISPLAY) > 1 // TFT LCD

  dp = new BB_SPI_LCD;
  dp->begin(TFT_TYPE);
  dp->allocBuffer(); // render all outputs to lib internal backbuffer
  dp->setRotation(
      MY_DISPLAY_FLIP ? 1 : 3); // 0 = no rotation, 1 = 90°, 2 = 180°, 3 = 270°
  dp->setTextColor(MY_DISPLAY_FGCOLOR, MY_DISPLAY_BGCOLOR);

#endif

  dp_clear();
  if (contrast)
    dp_contrast(contrast);
}

void dp_init(bool verbose) {
  dp_setup(DISPLAYCONTRAST);

  // show chip information
  if (verbose) {
#if (VERBOSE)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    dp_setFont(MY_FONT_NORMAL);
    dp->printf("** PAXCOUNTER **\r\n");
    dp->printf("Software v%s\r\n", PROGVERSION);
    dp->printf("ESP32 %d cores\r\n", chip_info.cores);
    dp->printf("Chip Rev.%d\r\n", chip_info.revision);
    dp->printf("WiFi%s%s\r\n",
               (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
               (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    dp->printf("%dMB %s Flash", spi_flash_get_chip_size() / (1024 * 1024),
               (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
    dp_dump();
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
    dp_setFont(MY_FONT_NORMAL);
    dp->setCursor(x_offset, 0);
    dp->printf("DEVEUI:\r\n");
    for (uint8_t i = 0; i <= 3; i++) {
      dp->setCursor(x_offset, i * 8 + 20);
      dp->printf("%4.4s", deveui + i * 4);
    }

    // give user some time to read or take picture
    dp_dump();
#if !(BOOTMENU)
    delay(8000);
#endif

#endif // HAS_LORA
  }    // verbose

  dp_power(cfg.screenon); // set display off if disabled
} // dp_init

// write display content to display buffer
// nextpage = true -> flip 1 page
void dp_refresh(bool nextPage) {
  struct count_payload_t count; // libpax count storage
  static uint8_t DisplayPage = 0;
  char timeState, strftime_buf[45];
  time_t now;
  struct tm timeinfo = {0};
#ifndef HAS_BUTTON
  static uint32_t framecounter = 0;
  const uint32_t flip_threshold = DISPLAYCYCLE * 1000 / DISPLAYREFRESH_MS;
#endif

  // if display is switched off we don't refresh it to relax cpu
  if (!DisplayIsOn && (DisplayIsOn == cfg.screenon))
    return;

  // set display on/off according to current device configuration
  if (DisplayIsOn != cfg.screenon) {
    DisplayIsOn = cfg.screenon;
    dp_power(cfg.screenon);
  }

#ifndef HAS_BUTTON
  // auto flip page if we are in unattended mode
  if (++framecounter > flip_threshold) {
    framecounter = 0;
    nextPage = true;
  }
#endif

  if (nextPage) {
    DisplayPage = (DisplayPage >= DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
    dp_clear();
  } else
    dp->setCursor(0, 0);

  switch (DisplayPage) {
    // page 0: pax + parameters overview
    // page 1: pax + lorawan parameters
    // page 2: pax + GPS lat/lon
    // page 3: BME280/680 values
    // page 4: timeofday
    // page 5: pax graph
    // page 6: power overview
    // page 7: blank screen

    // ---------- page 0: parameters overview ----------
  case DISPLAY_PAGE_PAX_PARAM_OVERVIEW:

    // show pax
    libpax_counter_count(&count);
    dp_setFont(MY_FONT_LARGE);
    dp->printf("%-8u", count.pax);

    dp_setFont(MY_FONT_SMALL);
    dp->setCursor(0, MY_DISPLAY_FIRSTLINE);

    // line 3: wifi + bluetooth counters
    // WIFI:abcde BLTH:abcde

    if (cfg.wifiscan)
      dp->printf("WIFI:%-5u", count.wifi_count);
    else
      dp->printf("WIFI:off  ");
    if (cfg.blescan)
      dp->printf("BLTH:%-5u", count.ble_count);
    else
      dp->printf(" BLTH:off");
    dp->printf("\r\n");

    // line 4: Battery + GPS status + Wifi channel
    // B:a.bcV Sats:ab ch:ab
#if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
    if (batt_level > 0)
      dp->printf("Batt:%3u%% ", batt_level);
    else
      dp->printf("No batt   ");
#else
    dp->printf("          ");
#endif
    dp->printf("chan:%02u\r\n", channel);

    // line 5: RSSI limiter + free memory
    // RLIM:abcd Mem:abcdKB
    dp->printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d", cfg.rssilimit);
    dp->printf(" Mem:%uKB\r\n", getFreeRAM() / 1024);

    // line 6: time + date
    // Wed Jan 12 21:49:08 *

#if (TIME_SYNC_INTERVAL)
    timeState = TimePulseTick ? ' ' : timeSetSymbols[timeSource];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    dp->printf("%.20s", strftime_buf);

// display inverse timeState if clock controller is enabled
#if (defined HAS_DCF77) || (defined HAS_IF482)
    dp_setFont(MY_FONT_SMALL, 1);
    dp->printf("%c\r\n", timeState);
    dp_setFont(MY_FONT_SMALL, 0);
#else
    dp->printf("%c\r\n", timeState);
#endif
#endif // TIME_SYNC_INTERVAL

    // line 7: LMIC status
    // yyyyyyyyyyyyy xx SFab

#if (HAS_LORA)
    // LMiC event display
    dp->printf("%-16s ", lmic_event_msg);
    // LORA datarate, display inverse if ADR disabled
    dp_setFont(MY_FONT_SMALL, !cfg.adrmode);
    dp->printf("%-4s", getSfName(updr2rps(LMIC.datarate)));
    dp_setFont(MY_FONT_SMALL, 0);
#endif // HAS_LORA

    dp_dump();
    break;

  // ---------- page 1: lorawan parameters ----------
  case DISPLAY_PAGE_PAX_LORAWAN_PARAM:

#if (HAS_LORA)

    // 3|Net:000000   Pwr:aa
    // 4|Dev:00000000 DR:0
    // 5|CHMsk:0000 Nonce:0000
    // 6|fUp:000000 fDn:000000
    // 7|SNR:-0000  RSSI:-0000

    // show pax
    libpax_counter_count(&count);
    dp_setFont(MY_FONT_LARGE);
    dp->printf("%-8u", count.pax);

    dp_setFont(MY_FONT_SMALL);
    dp->setCursor(0, MY_DISPLAY_FIRSTLINE);
    dp->printf("Net:%06X   Pwr:%2u\r\n", LMIC.netid & 0x001FFFFF,
               LMIC.radio_txpow);
    dp->printf("Dev:%08X DR:%1u\r\n", LMIC.devaddr, LMIC.datarate);
    dp->printf("ChMsk:%04X Nonce:%04X\r\n", LMIC.channelMap, LMIC.devNonce);
    dp->printf("fUp:%-6u fDn:%-6u\r\n", LMIC.seqnoUp ? LMIC.seqnoUp - 1 : 0,
               LMIC.seqnoDn ? LMIC.seqnoDn - 1 : 0);
    dp->printf("SNR:%-5d  RSSI:%-5d", (LMIC.snr + 2) / 4, LMIC.rssi);

    dp_dump();
    break;
#else  // skip this page
    DisplayPage++;
    break;
#endif // HAS_LORA

  // ---------- page 2: GPS ----------
  case DISPLAY_PAGE_PAX_GPS_LAT_LONG:

#if (HAS_GPS)

    // show pax
    libpax_counter_count(&count);
    dp_setFont(MY_FONT_LARGE);
    dp->printf("%-8u", count.pax);

    // show satellite status at bottom line
    dp_setFont(MY_FONT_SMALL);
    dp->setCursor(0, 56);
    dp->printf("%u Sats", gps.satellites.value());
    dp->printf(gps_hasfix() ? "         " : " - No fix");

    // show latitude and longitude
    dp_setFont(MY_FONT_STRETCHED);
    dp->setCursor(0, MY_DISPLAY_FIRSTLINE);
    if (gps.location.isUpdated()) {
      // When reading the GPS location, the updated flag is reset.
      // However if we also need to send the GPS location data, 
      // we must know whether there has been an update since the last time
      // we sent the location.
      gps_location_isupdated = true;
    }

    dp->printf("%c%09.6f\r\n", gps.location.rawLat().negative ? 'S' : 'N',
               gps.location.lat());
    dp->printf("%c%09.6f", gps.location.rawLng().negative ? 'W' : 'E',
               gps.location.lng());
    dp_dump();
    break;
#else // skip this page
    DisplayPage++;
    break;
#endif

  // ---------- page 3: BME280/680 ----------
  case DISPLAY_PAGE_BME280_680_VALUES:

#if (HAS_BME)
    dp_setFont(MY_FONT_STRETCHED);
    dp->setCursor(0, 0);
    dp->printf("TMP %-6.1f\r\n", bme_status.temperature);
    dp->printf("HUM %-6.1f\r\n", bme_status.humidity);
    dp->printf("PRS %-6.1f\r\n", bme_status.pressure);
#ifdef HAS_BME680
    dp->printf("IAQ %-6.0f", bme_status.iaq);
#endif
    dp_dump();
#else  // skip this page
    DisplayPage++;
#endif // HAS_BME
    break;

  // ---------- page 4: time ----------
  case DISPLAY_PAGE_TIME_OF_DAY:

    time(&now);
    localtime_r(&now, &timeinfo);

    dp_setFont(MY_FONT_STRETCHED);
    dp->setCursor(0, 0);
    dp->printf("Timeofday:");
    dp->setCursor(0, 26);
    dp_setFont(MY_FONT_LARGE);
    strftime(strftime_buf, sizeof(strftime_buf), "%T", &timeinfo);
    dp->printf("%.8s\r\n", strftime_buf);
    dp_setFont(MY_FONT_SMALL);
    dp->printf("%-12.1f", uptime() / 1000.0);
    dp_dump();
    break;

  // ---------- page 5: power overview ----------
  case DISPLAY_PAGE_POWER_OVERVIEW:
  {
#if defined(HAS_PMU) || defined(HAS_IP5306)

    dp_setFont(MY_FONT_STRETCHED); // 12x16px = 10 chars / line @ 4 lines
  
  #ifdef HAS_PMU
    if (pmu != nullptr) {
      if (pmu->isBatteryConnect()) {
        const float volt = static_cast<float>(pmu->getBattVoltage()) / 1000.0f;
        const bool charging = pmu->isCharging();
        const float current_mA = charging 
            ? AXPxxx_getBatteryChargeCurrent()
            : AXPxxx_getBattDischargeCurrent();

        const float power_mw = volt * current_mA;

        dp->printf("Bat %5.3fV\r\n",
                  volt);

        if (pmu->isVbusIn()) {
          dp_setFont(MY_FONT_NORMAL); // 8x8px = 16 chars / line @ 8 lines

          // When charging, no use of showing battery level
          dp->printf("%4.0fmA @ %4.0fmW \r\n%16s\r\n%16s\r\n", 
              current_mA,
              power_mw,
              charging ? "    charging    " : "      full      ",
              "");
        } else {
          // We have a few extra lines
          dp_setFont(MY_FONT_STRETCHED); // 12x16px = 10 chars / line @ 4 lines

          dp->printf("%8.0fmA \r\n%8.0fmW \r\n", 
              current_mA,
              power_mw);

          dp_setFont(MY_FONT_NORMAL); // 8x8px = 16 chars / line @ 8 lines

          if (!charging) {
            // Not charging with current_mA of 0mA is possible when the battery is nearly full
            // Make sure we're not showing "-0mA" when USB connected but not charging.
            dp->printf("%16s\r\n%3u%%%12s\r\n",
                "",
                pmu->getBatteryPercent(),
                (current_mA > 1.0f) ? "discharging" : "full");
          }
        }
      } else {
        dp->printf("%-10s\r\n%-10s\r\n", "", "No Battery");
      }

      // Do some averaging to make the displayed values less flipping around
      static float filtered_m_volt = -1.0f;
      static float filtered_m_amp  = -1.0f;
      static float filtered_m_watt = -1.0f;
      if (pmu->isVbusIn())
      {
        if (filtered_m_volt < 0.0f)
          filtered_m_volt = pmu->getVbusVoltage();
        if (filtered_m_amp < 0.0f)
          filtered_m_amp = AXPxxx_getVbusCurrent();
        if (filtered_m_watt < 0.0f)  
          filtered_m_watt = filtered_m_volt * filtered_m_amp / 1000.0f;
        const float m_volt = pmu->getVbusVoltage();
        const float m_amp  = AXPxxx_getVbusCurrent();
        filtered_m_volt = (3 * filtered_m_volt + m_volt) / 4;
        filtered_m_amp  = (3 * filtered_m_amp + m_amp) / 4;
        const float m_watt = (filtered_m_volt * filtered_m_amp) / 1000.0f;
        filtered_m_watt = (7 * filtered_m_watt + m_watt) / 8;

        dp_setFont(MY_FONT_STRETCHED); // 12x16px = 10 chars / line @ 4 lines

        dp->printf("USB %5.3fV \r\n",
                filtered_m_volt / 1000.0f);

        dp_setFont(MY_FONT_NORMAL); // 8x8px = 16 chars / line @ 8 lines
        dp->printf("%4.0fmA @ %4.0fmW \r\n",
                filtered_m_amp,
                filtered_m_watt);
      } else {
        // Filtering adds some delayed response, so make sure to 
        // force a full refresh when USB is reconnected
        filtered_m_volt = -1.0f;
        filtered_m_amp  = -1.0f;
        filtered_m_watt = -1.0f;
        dp_setFont(MY_FONT_STRETCHED); // 12x16px = 10 chars / line @ 4 lines
        dp->printf("%11s\r\n%11s\r\n", "", "");
      }
    }

  #else

    bool usb = IP5306_GetPowerSource();
    bool full = IP5306_GetBatteryFull();
    uint8_t level = IP5306_GetBatteryLevel();
    dp->printf("%-10s\r\n%-10s\r\nLvl: %3u%%\r\n",
            usb ? "USB" : "BATTERY", // Power source
            full ? "CHARGED" : (usb ? "CHARGING" : "DISCHARGE"), // State
            level); // Battery level

  #endif
    dp_dump();
#else  // skip this page
    DisplayPage++;
#endif // HAS_PMU
    break;
  }

  // ---------- page 6: pax graph ----------
  case DISPLAY_PAGE_PAX_GRAPH:

    // update and show histogram
    dp_plotCurve(count.pax, false);
    dp_dump(plotbuf);
    break;

  // ---------- page 7: blank screen ----------
  case DISPLAY_PAGE_BLANK_SCREEN:

#ifdef HAS_BUTTON
    dp_clear();
    break;
#else // skip this page and start over
    DisplayPage = 0;
    break;
#endif
  } // switch (page)
} // dp_refresh

// ------------- display helper functions -----------------

void dp_setFont(int font, int inv) {
  dp->setFont(font);
  if (inv)
    dp->setTextColor(MY_DISPLAY_BGCOLOR, MY_DISPLAY_FGCOLOR);
  else
    dp->setTextColor(MY_DISPLAY_FGCOLOR, MY_DISPLAY_BGCOLOR);
}

void dp_dump(uint8_t *pBuffer) {
  if (pBuffer)
    memcpy(dp->getBuffer(), pBuffer, PLOTBUFFERSIZE);
  dp->display();
}

void dp_clear(void) {
  dp->fillScreen(MY_DISPLAY_BGCOLOR);
  dp->display();
  dp->setCursor(0, 0);
}

void dp_contrast(uint8_t contrast) {
#if (HAS_DISPLAY) == 1
  dp->setContrast(contrast);
#elif (HAS_DISPLAY) > 1
  // to do: gamma correction for TFT
#endif
}

void dp_power(uint8_t screenon) {
#if (HAS_DISPLAY) == 1
  dp->setPower(screenon);
#elif (HAS_DISPLAY) > 1
  // to come
#endif
}

void dp_shutdown(void) {
#if (HAS_DISPLAY) == 1
  dp->setPower(false);
  delay(DISPLAYREFRESH_MS / 1000 * 1.1);
#elif (HAS_DISPLAY) > 1
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
        dp->fillRect(x * QR_SCALEFACTOR + offset_x,
                     y * QR_SCALEFACTOR + offset_y, QR_SCALEFACTOR,
                     QR_SCALEFACTOR, MY_DISPLAY_FGCOLOR);
  // draw horizontal frame lines
  dp->fillRect(0, 0, qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y,
               MY_DISPLAY_FGCOLOR);
  dp->fillRect(0, qrcode.size * QR_SCALEFACTOR + offset_y,
               qrcode.size * QR_SCALEFACTOR + 2 * offset_x, offset_y,
               MY_DISPLAY_FGCOLOR);
  // draw vertical frame lines
  dp->fillRect(0, 0, offset_x, qrcode.size * QR_SCALEFACTOR + 2 * offset_y,
               MY_DISPLAY_FGCOLOR);
  dp->fillRect(qrcode.size * QR_SCALEFACTOR + offset_x, 0, offset_x,
               qrcode.size * QR_SCALEFACTOR + 2 * offset_y, MY_DISPLAY_FGCOLOR);
}

// ------------- graphics primitives -----------------

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

  // nothing new to plot? -> then exit early
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
