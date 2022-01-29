// routines for writing data to an SD-card, if present

// Local logging tag
static const char TAG[] = __FILE__;

#include "sdcard.h"

#ifdef HAS_SDCARD

static bool useSDCard;

static void createFile(void);

File fileSDCard;

bool sdcard_close(void) {
  ESP_LOGD(TAG, "unmounting SD-card");
  fileSDCard.flush();
  fileSDCard.close();
return true;
}

bool sdcard_init(bool create) {
  ESP_LOGI(TAG, "looking for SD-card...");

  // for usage of SD drivers on ESP32 platform see
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdspi_host.html
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html

#if HAS_SDCARD == 1 // use SD SPI host driver
  useSDCard = SD.begin(SDCARD_CS, SDCARD_MOSI, SDCARD_MISO, SDCARD_SCLK);
#elif HAS_SDCARD == 2 // use SD MMC host driver
  // enable internal pullups of sd-data lines
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA0), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA1), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA2), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA3), GPIO_PULLUP_ONLY);
  useSDCard = SD_MMC.begin();
#endif

  if (useSDCard) {
    ESP_LOGI(TAG, "SD-card found");
    createFile();
  } else
    ESP_LOGI(TAG, "SD-card not found");
  return useSDCard;
}

void sdcardWriteData(uint16_t noWifi, uint16_t noBle,
                     __attribute__((unused)) uint16_t voltage) {
  static int counterWrites = 0;
  char tempBuffer[20 + 1];
  time_t t = time(NULL);
  struct tm tt;
  gmtime_r(&t, &tt); // make UTC timestamp

#if (HAS_SDS011)
  sdsStatus_t sds;
#endif

  if (!useSDCard)
    return;

  ESP_LOGD(TAG, "writing to SD-card");
  strftime(tempBuffer, sizeof(tempBuffer), "%FT%TZ", &tt);
  fileSDCard.print(tempBuffer);
  snprintf(tempBuffer, sizeof(tempBuffer), ",%d,%d", noWifi, noBle);
  fileSDCard.print(tempBuffer);
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
  snprintf(tempBuffer, sizeof(tempBuffer), ",%d", voltage);
  fileSDCard.print(tempBuffer);
#endif
#if (HAS_SDS011)
  sds011_store(&sds);
  snprintf(tempBuffer, sizeof(tempBuffer), ",%5.1f,%4.1f", sds.pm10, sds.pm25);
  fileSDCard.print(tempBuffer);
#endif
  fileSDCard.println();

  if (++counterWrites > 2) {
    // force writing to SD-card
    ESP_LOGD(TAG, "flushing data to card");
    fileSDCard.flush();
    counterWrites = 0;
  }
}

void createFile(void) {
  char bufferFilename[8 + 1 + 3 + 1];

  useSDCard = false;

  for (int i = 0; i < 100; i++) {
    sprintf(bufferFilename, SDCARD_FILE_NAME, i);
    ESP_LOGD(TAG, "SD: looking for file <%s>", bufferFilename);

#if HAS_SDCARD == 1
    bool fileExists = SD.exists(bufferFilename);
#elif HAS_SDCARD == 2
    bool fileExists = SD_MMC.exists(bufferFilename);
#endif

    if (!fileExists) {
      ESP_LOGD(TAG, "SD: file does not exist: creating");

#if HAS_SDCARD == 1
      fileSDCard = SD.open(bufferFilename, FILE_WRITE);
#elif HAS_SDCARD == 2
      fileSDCard = SD_MMC.open(bufferFilename, FILE_WRITE);
#endif

      if (fileSDCard) {
        ESP_LOGD(TAG, "SD: file opened: <%s>", bufferFilename);
        fileSDCard.print(SDCARD_FILE_HEADER);
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
        fileSDCard.print(SDCARD_FILE_HEADER_VOLTAGE); // for battery level data
#endif
#if (HAS_SDS011)
        fileSDCard.print(SDCARD_FILE_HEADER_SDS011);
#endif
        fileSDCard.println();
        useSDCard = true;
        break;
      }
    }
  }
  return;
}

#endif // (HAS_SDCARD)
