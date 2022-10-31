#ifdef HAS_SDCARD

// routines for writing data to an SD-card, if present
// use FAT32 formatted card
// check whether your card reader supports SPI oder SDMMC and select appropriate
// SD interface in board hal file

// Local logging tag
static const char TAG[] = __FILE__;

#include "sdcard.h"

sdmmc_card_t *card;

const char mount_point[] = MOUNT_POINT;
static bool useSDCard = false;

// This file stream will be used for payload logging
static FILE *data_file;
// This file stream will be used for system logging

#ifdef SD_LOGGING
static FILE *log_file;

// Save UART stdout stream
static FILE *uart_stdout = stdout;

// This function will be called by the ESP log library every time ESP_LOG needs
// to be performed.
//      @important Do NOT use the ESP_LOG* macro's in this function ELSE
//      recursive loop and stack overflow! So use printf() instead for debug
//      messages.
// CURRENTLY NOT WORKING DUE TO AN ISSUE IN ARDUINO-ESP32
int print_to_sd_card(const char *fmt, va_list args) {
  static bool static_fatal_error = false;
  static const uint32_t WRITE_CACHE_CYCLE = 5;
  static uint32_t counter_write = 0;
  int iresult;

  // #1 Write to file
  if (log_file == NULL) {
    printf("%s() ABORT. file handle log_file is NULL\n", __FUNCTION__);
    return -1;
  }
  if (static_fatal_error == false) {
    iresult = vfprintf(log_file, fmt, args);
    if (iresult < 0) {
      printf("%s() ABORT. failed vfprintf() -> logging disabled \n",
             __FUNCTION__);
      // MARK FATAL
      static_fatal_error = true;
      return iresult;
    }

    // #2 Smart commit after x writes
    counter_write++;
    if (counter_write % WRITE_CACHE_CYCLE == 0) {
      printf("%s() fsync'ing log file (WRITE_CACHE_CYCLE=%u)\n",
             WRITE_CACHE_CYCLE);
      fsync(fileno(log_file));
    }
  }

  // #3 ALWAYS Write to stdout!
  return vprintf(fmt, args);
}
#endif

bool openFile(FILE **fd, const char *filename) {
  char _filename[50];
  sprintf(_filename, "%s%s", MOUNT_POINT, filename);

  if ((*fd = fopen(_filename, "a")) == NULL) {
    ESP_LOGE(TAG, "file <%s> open error", _filename);
    return false;
  } else {
    ESP_LOGI(TAG, "file <%s> opened", _filename);
    return true;
  }
} // openfile

bool sdcard_init(bool create) {
  esp_err_t ret;

  // for usage of SD drivers on ESP32 platform see
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdspi_host.html
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_mount_config_t mount_config = {.format_if_mount_failed = false,
                                             .max_files = 5};

  ESP_LOGI(TAG, "looking for SD-card...");

#if (HAS_SDCARD == 1) // use SD interface in SPI host mode

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = (gpio_num_t)SDCARD_MOSI,
      .miso_io_num = (gpio_num_t)SDCARD_MISO,
      .sclk_io_num = (gpio_num_t)SDCARD_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };

  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if you want
  // to use these signals (if your board has them)
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t)SDCARD_CS;

  ret = spi_bus_initialize(SPI_HOST, &bus_cfg, 1);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to initialize SPI bus");
    return false;
  }

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config,
                                &card);

#elif (HAS_SDCARD == 2) // use SD interface in MMC host mode

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
  // has these signals.
  // Default config for SDMMC_HOST_DEFAULT (4-bit bus width, slot 1)
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html
  sdmmc_slot_config_t slot_config = SDCARD_SLOTCONFIG;

  // Set 1-line or 4-line SD mode (default is 1-line)
  slot_config.width = SDCARD_SLOTWIDTH;

  // Enable internal pullups on enabled pins. The internal pullups
  // are insufficient however, please make sure 10k external pullups are
  // connected on the bus. This is for debug / example purpose only.
  slot_config.flags |= SDCARD_PULLUP;

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config,
                                &card);

#endif

  // mount error handling
  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "failed to mount filesystem");
    } else {
      ESP_LOGE(TAG, "SD-card not found (%d)", ret);
    }
    return false;
  }

  // SD card is now initialized
  useSDCard = true;
  ESP_LOGI(TAG, "filesystem mounted");
  sdmmc_card_print_info(stdout, card);

  // open files for data and, optional, system logging
  char bufferFilename[50];

  snprintf(bufferFilename, sizeof(bufferFilename), "/%s.csv", SDCARD_FILE_NAME);

  if (openFile(&data_file, bufferFilename)) {
    fpos_t position;
    fgetpos(data_file, &position);

    // empty file? then we write a header line
    if (position == 0) {
      fprintf(data_file, "%s", SDCARD_FILE_HEADER);
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
      fprintf(data_file, "%s", SDCARD_FILE_HEADER_VOLTAGE);
#endif
#if (HAS_SDS011)
      fprintf(data_file, "%s", SDCARD_FILE_HEADER_SDS011);
#endif
    }
    fprintf(data_file, "\n");

  } else {
    useSDCard = false;
  }

#ifdef SD_LOGGING
  snprintf(bufferFilename, sizeof(bufferFilename), "/%s.log", SDCARD_FILE_NAME);

  if (openFile(&log_file, bufferFilename)) {
    ESP_LOGI(TAG, "redirecting serial output to SD-card");
    esp_log_set_vprintf(&print_to_sd_card);
    // Change stdout for THIS TASK ONLY
    // stdout = log_file;
    // Change stdout for all new tasks which will be created
    //_GLOBAL_REENT->_stdout = log_file;
  } else {
    useSDCard = false;
  }
#endif

  return useSDCard;
} // sdcard_init

void sdcard_flush(void) {
  if (data_file)
    fsync(fileno(data_file));
#ifdef SD_LOGGING
  if (log_file)
    fsync(fileno(log_file));
#endif
}

void sdcard_close(void) {
  ESP_LOGI(TAG, "closing SD-card");
  sdcard_flush();
#ifdef SD_LOGGING
  // Reset logging output back to normal
  ESP_LOGI(TAG, "redirect console back to serial output");
  // stdout = uart_stdout;
  //_GLOBAL_REENT->_stdout = uart_stdout;
  esp_log_set_vprintf(&vprintf);
#endif
  fcloseall();
  esp_vfs_fat_sdcard_unmount(mount_point, card);
  ESP_LOGI(TAG, "SD-card unmounted");
}

void sdcardWriteData(uint16_t noWifi, uint16_t noBle,
                     __attribute__((unused)) uint16_t voltage) {
  if (!useSDCard)
    return;

  char timeBuffer[20 + 1];
  time_t t = time(NULL);
  struct tm tt;
  gmtime_r(&t, &tt); // make UTC timestamp
  strftime(timeBuffer, sizeof(timeBuffer), "%FT%TZ", &tt);

#if (HAS_SDS011)
  sdsStatus_t sds;
#endif

  ESP_LOGI(TAG, "writing data to SD-card");

  fprintf(data_file, "%s", timeBuffer);
  fprintf(data_file, ",%d,%d", noWifi, noBle);
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
  fprintf(data_file, ",%d", voltage);
#endif
#if (HAS_SDS011)
  sds011_store(&sds);
  fprintf(data_file, ",%5.1f,%4.1f", sds.pm10 / 10, sds.pm25 / 10);
#endif
  fprintf(data_file, "\n");
}

#endif // (HAS_SDCARD)