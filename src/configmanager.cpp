/* configmanager persists runtime configuration using NVRAM of ESP32*/

#include "globals.h"
#include "configmanager.h"

// Local logging tag
static const char TAG[] = __FILE__;

// namespace for device runtime preferences
#define DEVCONFIG "paxcntcfg"

Preferences nvram;

configData_t cfg; // struct holds current device configuration

static const uint8_t cfgMagicBytes[] = {0x21, 0x76, 0x87, 0x32, 0xf4};
static const size_t cfgLen = sizeof(cfg), cfgLen2 = sizeof(cfgMagicBytes);
static uint8_t buffer[cfgLen + cfgLen2];

// populate runtime config with device factory settings
//
// configuration frame structure in NVRAM;
// 1. version header [10 bytes, containing version string]
// 2. user settings [cfgLen bytes, containing default runtime settings
// (configData_t cfg)]
// 3. magicByte [cfgLen2 bytes, containing a fixed identifier]

static void defaultConfig(configData_t *myconfig) {
  strncpy(myconfig->version, PROGVERSION,
          sizeof(myconfig->version) - 1); // Firmware version

  // device factory settings
  myconfig->loradr = LORADRDEFAULT; // 0-15, lora datarate, see paxcounter.conf
  myconfig->txpower = LORATXPOWDEFAULT; // 0-15, lora tx power
  myconfig->adrmode = 1;                // 0=disabled, 1=enabled
  myconfig->screensaver = 0;            // 0=disabled, 1=enabled
  myconfig->screenon = 1;               // 0=disabled, 1=enabled
  myconfig->countermode =
      COUNTERMODE;                 // 0=cyclic, 1=cumulative, 2=cyclic confirmed
  myconfig->rssilimit = RSSILIMIT; // threshold for rssilimiter, negative value!
  myconfig->sendcycle = SENDCYCLE; // payload send cycle [seconds/2]
  myconfig->sleepcycle = SLEEPCYCLE; // sleep cycle [seconds/10]
  myconfig->wifichancycle =
      WIFI_CHANNEL_SWITCH_INTERVAL; // wifi channel switch cycle [seconds/100]
  myconfig->blescantime =
      BLESCANINTERVAL /
      10; // BT channel scan cycle [seconds/100], default 1 (= 10ms)
  myconfig->blescan = BLECOUNTER;   // 0=disabled, 1=enabled
  myconfig->wifiscan = WIFICOUNTER; // 0=disabled, 1=enabled
  myconfig->wifiant = 0;            // 0=internal, 1=external (for LoPy/LoPy4)
  myconfig->rgblum = RGBLUMINOSITY; // RGB Led luminosity (0..100%)
  myconfig->payloadmask = PAYLOADMASK; // payloads as defined in default

#ifdef HAS_BME680
  // initial BSEC state for BME680 sensor
  myconfig->bsecstate[BSEC_MAX_STATE_BLOB_SIZE] = {0};
#endif
}

// migrate runtime configuration from earlier to current version
static void migrateConfig(void) {
  // currently no configuration migration rules are implemented, we reset to
  // factory settings instead
  eraseConfig();
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
void loadConfig(void) {
  int readBytes = 0;

  ESP_LOGI(TAG, "Loading device configuration from NVRAM...");

  if (nvram.begin(DEVCONFIG, true)) {
    // load device runtime config from nvram and copy it to byte array
    readBytes = nvram.getBytes(DEVCONFIG, buffer, cfgLen + cfgLen2);
    nvram.end();

    // check that runtime config data length matches
    if (readBytes != cfgLen + cfgLen2) {
      ESP_LOGE(TAG, "No valid configuration found");
      migrateConfig();
    }

  } else {
    ESP_LOGI(TAG, "NVRAM initialized, device starts with factory settings");
    eraseConfig();
  }

  // validate loaded configuration by checking magic bytes at end of array
  if (memcmp(buffer + cfgLen, &cfgMagicBytes, cfgLen2) != 0) {
    ESP_LOGE(TAG, "Configuration data corrupt");
    eraseConfig();
  }

  // copy loaded configuration into runtime cfg struct
  memcpy(&cfg, buffer, cfgLen);
  ESP_LOGI(TAG, "Runtime configuration v%s loaded", cfg.version);

  // check if config version matches current firmware version
  switch (version_compare(PROGVERSION, cfg.version)) {
  case -1: // device configuration belongs to newer than current firmware
    ESP_LOGE(TAG, "Incompatible device configuration");
    eraseConfig();
    break;
  case 1: // device configuration belongs to older than current firmware
    ESP_LOGW(TAG, "Device was updated, attempt to migrate configuration");
    migrateConfig();
    break;
  default: // device configuration version matches current firmware version
    break; // nothing to do here
  }
}

// helper function to convert strings into lower case
bool comp(char s1, char s2) { return (tolower(s1) < tolower(s2)); }

// helper function to lexicographically compare two versions. Returns 1 if v2
// is smaller, -1 if v1 is smaller, 0 if equal
int version_compare(const String v1, const String v2) {
  if (v1 == v2)
    return 0;

  const char *a1 = v1.c_str(), *a2 = v2.c_str();

  if (std::lexicographical_compare(a1, a1 + strlen(a1), a2, a2 + strlen(a2),
                                   comp))
    return -1;
  else
    return 1;
}

void eraseConfig(void) {
  reset_rtc_vars();
  saveConfig(true);
}