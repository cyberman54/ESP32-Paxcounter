// routines for writing data to an SD-card, if present

#if (HAS_SDCARD)

// Local logging tag
static const char TAG[] = __FILE__;

#include "sdcard.h"

static bool useSDCard;

static void createFile(void);

File fileSDCard;

bool sdcardInit() {
  ESP_LOGD(TAG, "looking for SD-card...");
  useSDCard = SD.begin(SDCARD_CS, SDCARD_MOSI, SDCARD_MISO, SDCARD_SCLK);
  if (useSDCard)
    createFile();
  return useSDCard;
}

void sdcardWriteData(uint16_t noWifi, uint16_t noBle) {
  static int counterWrites = 0;
  char tempBuffer[12 + 1];
  time_t t = now();

  if (!useSDCard)
    return;

  ESP_LOGD(TAG, "writing to SD-card");
  sprintf(tempBuffer, "%02d.%02d.%4d,", day(t), month(t), year(t));
  fileSDCard.print(tempBuffer);
  sprintf(tempBuffer, "%02d:%02d:%02d,", hour(t), minute(t), second(t));
  fileSDCard.print(tempBuffer);
  sprintf(tempBuffer, "%d,%d", noWifi, noBle);
  fileSDCard.println(tempBuffer);

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
    bool fileExists = SD.exists(bufferFilename);
    if (!fileExists) {
      ESP_LOGD(TAG, "SD: file does not exist: opening");
      fileSDCard = SD.open(bufferFilename, FILE_WRITE);
      if (fileSDCard) {
        ESP_LOGD(TAG, "SD: name opended: <%s>", bufferFilename);
        fileSDCard.println(SDCARD_FILE_HEADER);
        useSDCard = true;
        break;
      }
    }
  }
  return;
}

#endif // (HAS_SDCARD)