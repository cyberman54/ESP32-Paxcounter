// routines for writing data to an SD-card, if present
// use FAT32 formatted card
// check whether your card reader supports SPI oder SDMMC and select appropriate
// SD low level driver in board hal file

// Local logging tag
static const char TAG[] = __FILE__;

#include "sdcard.h"

#ifdef HAS_SDCARD

static bool useSDCard;
static void openFile(void);

File fileSDCard;

#if HAS_SDCARD == 1
SPIClass sd_spi;
#endif

bool sdcard_init(bool create) {

  // for usage of SD drivers on ESP32 platform see
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdspi_host.html
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html

  ESP_LOGI(TAG, "looking for SD-card...");
#if HAS_SDCARD == 1 // use SD SPI host driver
  digitalWrite(SDCARD_CS, HIGH);
  sd_spi.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  digitalWrite(SDCARD_CS, LOW);
  useSDCard = SD.begin(SDCARD_CS, sd_spi);
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
    openFile();
    return true;
  } else {
    ESP_LOGI(TAG, "SD-card not found");
    return false;
  }
}

void sdcard_close(void) {
  ESP_LOGI(TAG, "closing SD-card");
  fileSDCard.flush();
  fileSDCard.close();
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

  ESP_LOGI(TAG, "SD: writing data");
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
  snprintf(tempBuffer, sizeof(tempBuffer), ",%5.1f,%4.1f", sds.pm10 / 10,
           sds.pm25 / 10);
  fileSDCard.print(tempBuffer);
#endif
  fileSDCard.println();

  if (++counterWrites > 2) {
    // force writing to SD-card
    ESP_LOGI(TAG, "SD: flushing data");
    fileSDCard.flush();
    counterWrites = 0;
  }
}

void openFile(void) {
  char bufferFilename[30];

  useSDCard = false;

  snprintf(bufferFilename, sizeof(bufferFilename), "/%s.csv", SDCARD_FILE_NAME);
  ESP_LOGI(TAG, "SD: looking for file <%s>", bufferFilename);

#if HAS_SDCARD == 1
  bool fileExists = SD.exists(bufferFilename);
#elif HAS_SDCARD == 2
  bool fileExists = SD_MMC.exists(bufferFilename);
#endif

  // file not exists, create it
  if (!fileExists) {
    ESP_LOGD(TAG, "SD: file not found, creating...");

#if HAS_SDCARD == 1
    fileSDCard = SD.open(bufferFilename, FILE_WRITE);
#elif HAS_SDCARD == 2
    fileSDCard = SD_MMC.open(bufferFilename, FILE_WRITE);
#endif

    if (fileSDCard) {
      ESP_LOGD(TAG, "SD: name opened: <%s>", bufferFilename);
      fileSDCard.print(SDCARD_FILE_HEADER);
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
      fileSDCard.print(SDCARD_FILE_HEADER_VOLTAGE); // for battery level data
#endif
#if (HAS_SDS011)
      fileSDCard.print(SDCARD_FILE_HEADER_SDS011);
#endif
      fileSDCard.println();
      useSDCard = true;
    }
  }

  // file exists, append data
  else {
    ESP_LOGD(TAG, "SD: file found, opening...");

#if HAS_SDCARD == 1
    fileSDCard = SD.open(bufferFilename, FILE_APPEND);
#elif HAS_SDCARD == 2
    fileSDCard = SD_MMC.open(bufferFilename, FILE_APPEND);
#endif

    if (fileSDCard) {
      ESP_LOGD(TAG, "SD: name opened: <%s>", bufferFilename);
      useSDCard = true;
    }
  }

  return;
}

#endif // (HAS_SDCARD)
