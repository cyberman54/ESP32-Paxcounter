#ifdef HAS_DISPLAY

// Basic Config
#include "globals.h"
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

HAS_DISPLAY u8x8(MY_OLED_RST, MY_OLED_SCL, MY_OLED_SDA);

// helper string for converting LoRa spread factor values
#if defined(CFG_eu868)
const char lora_datarate[] = {"1211100908077BFSNA"};
#elif defined(CFG_us915)
const char lora_datarate[] = {"100908078CNA121110090807"};
#endif

uint8_t volatile DisplayState = 0;

// helper function, prints a hex key on display
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb) {
  const uint8_t *p;
  for (uint8_t i = 0; i < len; i++) {
    p = lsb ? key + len - i - 1 : key + i;
    u8x8.printf("%02X", *p);
  }
  u8x8.printf("\n");
}

void init_display(const char *Productname, const char *Version) {

  // show startup screen
  uint8_t buf[32];
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
  u8x8.setFlipMode(0);
  u8x8.setInverseFont(1);
  u8x8.draw2x2String(0, 0, Productname);
  u8x8.setInverseFont(0);
  u8x8.draw2x2String(2, 2, Productname);
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  u8x8.clear();
  u8x8.setFlipMode(1);
  u8x8.setInverseFont(1);
  u8x8.draw2x2String(0, 0, Productname);
  u8x8.setInverseFont(0);
  u8x8.draw2x2String(2, 2, Productname);
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  u8x8.setFlipMode(0);
  u8x8.clear();

#ifdef DISPLAY_FLIP
  u8x8.setFlipMode(1);
#endif

// Display chip information
#ifdef VERBOSE
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  u8x8.printf("ESP32 %d cores\nWiFi%s%s\n", chip_info.cores,
              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  u8x8.printf("ESP Rev.%d\n", chip_info.revision);
  u8x8.printf("%dMB %s Flash\n", spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
#endif // VERBOSE

  u8x8.print(Productname);
  u8x8.print(" v");
  u8x8.println(PROGVERSION);

#ifdef HAS_LORA
  u8x8.println("DEVEUI:");
  os_getDevEui((u1_t *)buf);
  DisplayKey(buf, 8, true);
#endif // HAS_LORA

  vTaskDelay(3000 / portTICK_PERIOD_MS);
  u8x8.clear();
  u8x8.setPowerSave(!cfg.screenon); // set display off if disabled
  u8x8.draw2x2String(0, 0, "PAX:0");
#ifdef BLECOUNTER
  u8x8.setCursor(0, 3);
  u8x8.printf("BLTH:0");
#endif
  u8x8.setCursor(0, 4);
  u8x8.printf("WIFI:0");
  u8x8.setCursor(0, 5);
  u8x8.printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%d", cfg.rssilimit);

} // init_display

void refreshtheDisplay() {

  // set display on/off according to current device configuration
  if (DisplayState != cfg.screenon) {
    DisplayState = cfg.screenon;
    u8x8.setPowerSave(!cfg.screenon);
  }

  // if display is switched off we don't refresh it and save time
  if (!DisplayState)
    return;

  uint8_t msgWaiting;
  char buff[16]; // 16 chars line buffer

  // update counter (lines 0-1)
  snprintf(
      buff, sizeof(buff), "PAX:%-4d",
      (int)macs.size()); // convert 16-bit MAC counter to decimal counter value
  u8x8.draw2x2String(0, 0,
                     buff); // display number on unique macs total Wifi + BLE

// update Battery status (line 2)
#ifdef HAS_BATTERY_PROBE
  u8x8.setCursor(0, 2);
  u8x8.printf(batt_voltage > 4000 ? "B:USB " : "B:%.1fV",
              batt_voltage / 1000.0);
#endif

// update GPS status (line 2)
#ifdef HAS_GPS
  u8x8.setCursor(9, 2);
  if (!gps.location.isValid()) // if no fix then display Sats value inverse
  {
    u8x8.setInverseFont(1);
    u8x8.printf("Sats:%.2d", gps.satellites.value());
    u8x8.setInverseFont(0);
  } else
    u8x8.printf("Sats:%.2d", gps.satellites.value());
#endif

    // update bluetooth counter + LoRa SF (line 3)
#ifdef BLECOUNTER
  u8x8.setCursor(0, 3);
  if (cfg.blescan)
    u8x8.printf("BLTH:%-4d", macs_ble);
  else
    u8x8.printf("%s", "BLTH:off");
#endif

#ifdef HAS_LORA
  u8x8.setCursor(11, 3);
  u8x8.printf("SF:");
  if (cfg.adrmode) // if ADR=on then display SF value inverse
    u8x8.setInverseFont(1);
  u8x8.printf("%c%c", lora_datarate[LMIC.datarate * 2],
              lora_datarate[LMIC.datarate * 2 + 1]);
  if (cfg.adrmode) // switch off inverse if it was turned on
    u8x8.setInverseFont(0);
#endif // HAS_LORA

  // update wifi counter + channel display (line 4)
  u8x8.setCursor(0, 4);
  u8x8.printf("WIFI:%-4d", macs_wifi);
  u8x8.setCursor(11, 4);
  u8x8.printf("ch:%02d", channel);

  // update RSSI limiter status & free memory display (line 5)
  u8x8.setCursor(0, 5);
  u8x8.printf(!cfg.rssilimit ? "RLIM:off " : "RLIM:%-4d", cfg.rssilimit);
  u8x8.setCursor(10, 5);
  u8x8.printf("%4dKB", ESP.getFreeHeap() / 1024);

#ifdef HAS_LORA
  // update LoRa status display (line 6)
  u8x8.setCursor(0, 6);
  u8x8.printf("%-16s", display_line6);

  // update LMiC event display (line 7)
  u8x8.setCursor(0, 7);
  u8x8.printf("%-14s", display_line7);

  // update LoRa send queue display (line 7)
  msgWaiting = uxQueueMessagesWaiting(LoraSendQueue);
  if (msgWaiting) {
    sprintf(buff, "%2d", msgWaiting);
    u8x8.setCursor(14, 7);
    u8x8.setInverseFont(1);
    u8x8.printf("%-2s", msgWaiting == SEND_QUEUE_SIZE ? "<>" : buff);
    u8x8.setInverseFont(0);
  } else
    u8x8.printf("  ");

#endif // HAS_LORA

} // refreshDisplay()

#endif // HAS_DISPLAY