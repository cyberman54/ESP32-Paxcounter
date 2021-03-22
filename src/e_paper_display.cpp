#ifdef HAS_E_PAPER_DISPLAY

#include "globals.h"
#include "e_paper_display.h"
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <e_paper_choose_board.h>
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

// local Tag for logging
static const char TAG[] = __FILE__;

const char *printmonth[] = {
  "xxx",
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec"
};

uint8_t E_paper_displayIsOn = 0;

void ePaper_init(bool verbose) {
  display.init();
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();

  if (verbose) {

    // show chip information
    #if (VERBOSE)
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);

      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(0, 10);
        display.println(String("Software v") + PROGVERSION);
        display.setCursor(0, 20);
        display.println(String("ESP32 ") + chip_info.cores + " cores");
        display.setCursor(0, 30);
        display.println(String("Chip Rev.") + chip_info.revision);
        display.setCursor(0, 40);
        display.println(
          String("WiFi") + 
          ((chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "") +
          ((chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "")
        );
        display.setCursor(0, 50);
        display.println(
          String(spi_flash_get_chip_size() / (1024 * 1024)) + 
          "Mb " +
          ((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.") +
          " Flash"
        );
      }
      while (display.nextPage());

      delay(3000);
    #endif // VERBOSE
  }

  draw_page();
  display.hibernate();
}
void refresh_ePaperDisplay(bool nextPage) {
  draw_page();
}

void draw_page() {
  const time_t t = myTZ.toLocal(now());

  char date_string [12];
  sprintf (date_string, "%02d.%3s %4d", day(t), printmonth[month(t)], year(t));
  int16_t dateX = calc_x_rightAlignment(date_string, 10);

  char time_string [5];
  sprintf (time_string, "%02d:%02d", hour(t), minute(t));
  int16_t timeX = calc_x_rightAlignment(time_string, 20);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0, 10);
    display.setTextSize(3);
    display.println(macs.size());

    display.setTextSize(1);

    display.setCursor(dateX, 10);
    display.println(date_string);

    display.setCursor(timeX, 20);
    display.println(time_string);
  }
  while (display.nextPage());
}

int calc_x_rightAlignment(String str, int16_t y) {
  int16_t x = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  
  return display.width() - w - x1;
}
#endif