/* configmanager persists runtime configuration using NVRAM of ESP32*/

#include "globals.h"
#include "configmanager.h"

// Local logging tag
static const char TAG[] = __FILE__;

#define PAYLOADMASK                                                            \
  ((GPS_DATA | ALARM_DATA | MEMS_DATA | COUNT_DATA | SENSOR1_DATA |            \
    SENSOR2_DATA | SENSOR3_DATA) &                                             \
   (~BATT_DATA))

// namespace for device runtime preferences
#define DEVCONFIG "paxcntcfg"

Preferences nvram;

static const char cfgMagicBytes[] = {0x21, 0x76, 0x87, 0x32, 0xf3};
static const size_t cfgLen = sizeof(cfg), cfgLen2 = sizeof(cfgMagicBytes);
static char buffer[cfgLen + cfgLen2];

// populate runtime config with factory settings
static void defaultConfig(configData_t *myconfig) {
  char version[10];
  snprintf(version, 10, "%-10s", PROGVERSION);

  // factory settings
  myconfig->loradr = LORADRDEFAULT; // 0-15, lora datarate, see paxcounter.conf
  myconfig->txpower = LORATXPOWDEFAULT; // 0-15, lora tx power
  myconfig->adrmode = 1;                // 0=disabled, 1=enabled
  myconfig->screensaver = 0;            // 0=disabled, 1=enabled
  myconfig->screenon = 1;               // 0=disabled, 1=enabled
  myconfig->countermode =
      COUNTERMODE;                 // 0=cyclic, 1=cumulative, 2=cyclic confirmed
  myconfig->rssilimit = 0;         // threshold for rssilimiter, negative value!
  myconfig->sendcycle = SENDCYCLE; // payload send cycle [seconds/2]
  myconfig->wifichancycle =
      WIFI_CHANNEL_SWITCH_INTERVAL; // wifi channel switch cycle [seconds/100]
  myconfig->blescantime =
      BLESCANINTERVAL /
      10; // BT channel scan cycle [seconds/100], default 1 (= 10ms)
  myconfig->blescan = 1;  // 0=disabled, 1=enabled
  myconfig->wifiscan = 1; // 0=disabled, 1=enabled
  myconfig->wifiant = 0;  // 0=internal, 1=external (for LoPy/LoPy4)
  myconfig->vendorfilter = VENDORFILTER;  // 0=disabled, 1=enabled
  myconfig->rgblum = RGBLUMINOSITY;       // RGB Led luminosity (0..100%)
  myconfig->monitormode = 0;              // 0=disabled, 1=enabled
  myconfig->payloadmask = PAYLOADMASK;    // all payload switched on
  myconfig->enscount =0; // 0= disabled, 1 = enabled
  memcpy(myconfig->version, version, 10); // Firmware version [exactly 10 chars]

#ifdef HAS_BME680
  // initial BSEC state for BME680 sensor
  myconfig->bsecstate[BSEC_MAX_STATE_BLOB_SIZE] = {0};
#endif
}

// save current configuration from RAM to NVRAM
void saveConfig(bool erase) {
  ESP_LOGI(TAG, "Storing settings to NVRAM...");

  nvram.begin(DEVCONFIG, false);

  if (erase) {
    ESP_LOGI(TAG, "Resetting device to factory settings");
    nvram.clear();
    defaultConfig(&cfg);
  }

  // Copy device runtime config cfg to byte array, padding it with magicBytes
  memcpy(buffer, &cfg, cfgLen);
  memcpy(buffer + cfgLen, &cfgMagicBytes, cfgLen2);

  // save byte array to NVRAM, padding with cfg magicbyes
  if (nvram.putBytes(DEVCONFIG, buffer, cfgLen + cfgLen2))
    ESP_LOGI(TAG, "Device settings saved");
  else
    ESP_LOGE(TAG, "NVRAM Error, device settings not saved");

  nvram.end();
}

// load configuration from NVRAM into RAM and make it current
bool loadConfig() {

  ESP_LOGI(TAG, "Loading device runtime configuration from NVRAM...");

  if (!nvram.begin(DEVCONFIG, true)) {
    ESP_LOGW(TAG, "NVRAM initialized, device starts with factory settings");
    eraseConfig();
    return true;

  } else {
    // simple check that runtime config data matches
    if (nvram.getBytesLength(DEVCONFIG) != (cfgLen + cfgLen2)) {
      ESP_LOGE(TAG, "configuration invalid");
      return false;

    } else {
      // load device runtime config from nvram and copy it to byte array
      nvram.getBytes(DEVCONFIG, buffer, cfgLen + cfgLen2);
      nvram.end();
      // validate configuration by checking magic bytes at end of array
      if (memcmp(buffer + cfgLen, &cfgMagicBytes, cfgLen2) != 0) {
        ESP_LOGW(TAG, "No configuration found");
        return false;

      } else {
        // copy byte array into runtime cfg struct
        memcpy(&cfg, buffer, cfgLen);
        ESP_LOGI(TAG, "Runtime configuration loaded");
        return true;
      }
    }
  }
}

void eraseConfig(void) { saveConfig(true); }
