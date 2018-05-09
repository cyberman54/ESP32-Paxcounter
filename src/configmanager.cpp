/* configmanager persists runtime configuration using NVRAM of ESP32*/

#include "globals.h"
#include <nvs.h>
#include <nvs_flash.h>

// Local logging tag
static const char *TAG = "configmanager";

nvs_handle my_handle;

esp_err_t err;

// defined in antenna.cpp
#ifdef HAS_ANTENNA_SWITCH
    void antenna_select(const uint8_t _ant);
#endif

// populate cfg vars with factory settings
void defaultConfig() {
    cfg.lorasf        = LORASFDEFAULT;  // 7-12, initial lora spreadfactor defined in paxcounter.conf
    cfg.txpower       = 15;             // 2-15, lora tx power
    cfg.adrmode       = 1;              // 0=disabled, 1=enabled
    cfg.screensaver   = 0;              // 0=disabled, 1=enabled
    cfg.screenon      = 1;              // 0=disbaled, 1=enabled
    cfg.countermode   = 0;              // 0=cyclic, 1=cumulative, 2=cyclic confirmed
    cfg.rssilimit     = 0;              // threshold for rssilimiter, negative value!
    cfg.sendcycle     = SEND_SECS;      // payload send cycle [seconds/2]
    cfg.wifichancycle = WIFI_CHANNEL_SWITCH_INTERVAL; // wifi channel switch cycle [seconds/100]
    cfg.blescantime   = BLESCANTIME;    // BLE scan cycle duration [seconds]
    cfg.blescan       = 1;              // 0=disabled, 1=enabled
    cfg.wifiant       = 0;              // 0=internal, 1=external (for LoPy/LoPy4)
    cfg.vendorfilter  = 1;              // 0=disabled, 1=enabled
    cfg.rgblum        = RGBLUMINOSITY;  // RGB Led luminosity (0..100%)

    strncpy( cfg.version, PROGVERSION, sizeof(cfg.version)-1 );
}

void open_storage() {
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    ESP_LOGI(TAG, "Opening NVS");
    err = nvs_open("config", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
      ESP_LOGI(TAG, "Error (%d) opening NVS handle", err);
    else
      ESP_LOGI(TAG, "Done");
}

// erase all keys and values in NVRAM
void eraseConfig() {
    ESP_LOGI(TAG, "Clearing settings in NVS");
    open_storage();
    if (err == ESP_OK) {
      nvs_erase_all(my_handle);
      nvs_commit(my_handle);
      nvs_close(my_handle);
      ESP_LOGI(TAG, "Done");}
    else {
      ESP_LOGW(TAG, "NVS erase failed"); }
}

// save current configuration from RAM to NVRAM
void saveConfig() {
    ESP_LOGI(TAG, "Storing settings in NVS");
    open_storage();
    if (err == ESP_OK) {
      int8_t  flash8  = 0;
      int16_t flash16 = 0;
      size_t required_size;
      char storedversion[10];

      if( nvs_get_str(my_handle, "version", storedversion, &required_size) != ESP_OK || strcmp(storedversion, cfg.version) != 0 )
        nvs_set_str(my_handle, "version", cfg.version);

      if( nvs_get_i8(my_handle, "lorasf", &flash8) != ESP_OK || flash8 != cfg.lorasf )
        nvs_set_i8(my_handle, "lorasf", cfg.lorasf);

      if( nvs_get_i8(my_handle, "txpower", &flash8) != ESP_OK || flash8 != cfg.txpower )
        nvs_set_i8(my_handle, "txpower", cfg.txpower);

      if( nvs_get_i8(my_handle, "adrmode", &flash8) != ESP_OK || flash8 != cfg.adrmode )
        nvs_set_i8(my_handle, "adrmode", cfg.adrmode);

      if( nvs_get_i8(my_handle, "screensaver", &flash8) != ESP_OK || flash8 != cfg.screensaver )
        nvs_set_i8(my_handle, "screensaver", cfg.screensaver);

      if( nvs_get_i8(my_handle, "screenon", &flash8) != ESP_OK || flash8 != cfg.screenon )
        nvs_set_i8(my_handle, "screenon", cfg.screenon);

      if( nvs_get_i8(my_handle, "countermode", &flash8) != ESP_OK || flash8 != cfg.countermode )
        nvs_set_i8(my_handle, "countermode", cfg.countermode);

      if( nvs_get_i8(my_handle, "sendcycle", &flash8) != ESP_OK || flash8 != cfg.sendcycle )
        nvs_set_i8(my_handle, "sendcycle", cfg.sendcycle);

      if( nvs_get_i8(my_handle, "wifichancycle", &flash8) != ESP_OK || flash8 != cfg.wifichancycle )
        nvs_set_i8(my_handle, "wifichancycle", cfg.wifichancycle);

      if( nvs_get_i8(my_handle, "blescantime", &flash8) != ESP_OK || flash8 != cfg.blescantime )
        nvs_set_i8(my_handle, "blescantime", cfg.blescantime);

      if( nvs_get_i8(my_handle, "blescanmode", &flash8) != ESP_OK || flash8 != cfg.blescan )
        nvs_set_i8(my_handle, "blescanmode", cfg.blescan);

      if( nvs_get_i8(my_handle, "wifiant", &flash8) != ESP_OK || flash8 != cfg.wifiant )
        nvs_set_i8(my_handle, "wifiant", cfg.wifiant);

      if( nvs_get_i8(my_handle, "vendorfilter", &flash8) != ESP_OK || flash8 != cfg.vendorfilter )
        nvs_set_i8(my_handle, "vendorfilter", cfg.vendorfilter);

      if( nvs_get_i8(my_handle, "rgblum", &flash8) != ESP_OK || flash8 != cfg.rgblum )
          nvs_set_i8(my_handle, "rgblum", cfg.rgblum);

      if( nvs_get_i16(my_handle, "rssilimit", &flash16) != ESP_OK || flash16 != cfg.rssilimit )
        nvs_set_i16(my_handle, "rssilimit", cfg.rssilimit);

      err = nvs_commit(my_handle);
      nvs_close(my_handle);
      if ( err == ESP_OK ) {
        ESP_LOGI(TAG, "Done");
      } else {
        ESP_LOGW(TAG, "NVS config write failed");
      }
    } else {
      ESP_LOGW(TAG, "Error (%d) opening NVS handle", err);
    }
}

// set and save cfg.version
void migrateVersion() {
  sprintf(cfg.version, "%s", PROGVERSION);
  ESP_LOGI(TAG, "version set to %s", cfg.version);
  saveConfig();
}

// load configuration from NVRAM into RAM and make it current
void loadConfig() {
  defaultConfig(); // start with factory settings
  ESP_LOGI(TAG, "Reading settings from NVS");
  open_storage();
  if (err != ESP_OK) {
    ESP_LOGW(TAG,"Error (%d) opening NVS handle, storing defaults", err);
    saveConfig(); } // saves factory settings to NVRAM
  else {
    int8_t  flash8  = 0;
    int16_t flash16 = 0;
    size_t required_size;

    // check if configuration stored in NVRAM matches PROGVERSION
    if( nvs_get_str(my_handle, "version", NULL, &required_size) == ESP_OK ) {
      nvs_get_str(my_handle, "version", cfg.version, &required_size);
      ESP_LOGI(TAG, "NVRAM settings version = %s", cfg.version);
      if (strcmp(cfg.version, PROGVERSION)) {
        ESP_LOGI(TAG, "migrating NVRAM settings to new version %s", PROGVERSION);
        nvs_close(my_handle);
        migrateVersion();
      }
    } else {
        ESP_LOGI(TAG, "new version %s, deleting NVRAM settings", PROGVERSION);
        nvs_close(my_handle);
        eraseConfig();
        migrateVersion();
    }

    // overwrite defaults with valid values from NVRAM
    if( nvs_get_i8(my_handle, "lorasf", &flash8) == ESP_OK ) {
      cfg.lorasf = flash8;
      ESP_LOGI(TAG, "lorasf = %d", flash8);
    } else {
      ESP_LOGI(TAG, "lorasf set to default %d", cfg.lorasf);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "txpower", &flash8) == ESP_OK ) {
      cfg.txpower = flash8;
      ESP_LOGI(TAG, "txpower = %d", flash8);
    } else {
      ESP_LOGI(TAG, "txpower set to default %d", cfg.txpower);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "adrmode", &flash8) == ESP_OK ) {
      cfg.adrmode = flash8;
      ESP_LOGI(TAG, "adrmode = %d", flash8);
    } else {
      ESP_LOGI(TAG, "adrmode set to default %d", cfg.adrmode);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "screensaver", &flash8) == ESP_OK ) {
      cfg.screensaver = flash8;
      ESP_LOGI(TAG, "screensaver = %d", flash8);
    } else {
      ESP_LOGI(TAG, "screensaver set to default %d", cfg.screensaver);
      saveConfig();
    }

     if( nvs_get_i8(my_handle, "screenon", &flash8) == ESP_OK ) {
      cfg.screenon = flash8;
      ESP_LOGI(TAG, "screenon = %d", flash8);
    } else {
      ESP_LOGI(TAG, "screenon set to default %d", cfg.screenon);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "countermode", &flash8) == ESP_OK ) {
      cfg.countermode = flash8;
      ESP_LOGI(TAG, "countermode = %d", flash8);
    } else {
      ESP_LOGI(TAG, "countermode set to default %d", cfg.countermode);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "sendcycle", &flash8) == ESP_OK ) {
      cfg.sendcycle = flash8;
      ESP_LOGI(TAG, "sendcycle = %d", flash8);
    } else {
      ESP_LOGI(TAG, "Payload send cycle set to default %d", cfg.sendcycle);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "wifichancycle", &flash8) == ESP_OK ) {
      cfg.wifichancycle = flash8;
      ESP_LOGI(TAG, "wifichancycle = %d", flash8);
    } else {
      ESP_LOGI(TAG, "WIFI channel cycle set to default %d", cfg.wifichancycle);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "wifiant", &flash8) == ESP_OK ) {
      cfg.wifiant = flash8;
      ESP_LOGI(TAG, "wifiantenna = %d", flash8);
    } else {
      ESP_LOGI(TAG, "WIFI antenna switch set to default %d", cfg.wifiant);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "vendorfilter", &flash8) == ESP_OK ) {
      cfg.vendorfilter = flash8;
      ESP_LOGI(TAG, "vendorfilter = %d", flash8);
    } else {
      ESP_LOGI(TAG, "Vendorfilter mode set to default %d", cfg.vendorfilter);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "rgblum", &flash8) == ESP_OK ) {
      cfg.rgblum = flash8;
      ESP_LOGI(TAG, "rgbluminosity = %d", flash8);
    } else {
      ESP_LOGI(TAG, "RGB luminosity set to default %d", cfg.rgblum);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "blescantime", &flash8) == ESP_OK ) {
      cfg.blescantime = flash8;
      ESP_LOGI(TAG, "blescantime = %d", flash8);
    } else {
      ESP_LOGI(TAG, "BLEscantime set to default %d", cfg.blescantime);
      saveConfig();
    }

    if( nvs_get_i8(my_handle, "blescanmode", &flash8) == ESP_OK ) {
      cfg.blescan = flash8;
      ESP_LOGI(TAG, "BLEscanmode = %d", flash8);
    } else {
      ESP_LOGI(TAG, "BLEscanmode set to default %d", cfg.blescan);
      saveConfig();
    }

    if( nvs_get_i16(my_handle, "rssilimit", &flash16) == ESP_OK ) {
      cfg.rssilimit = flash16;
      ESP_LOGI(TAG, "rssilimit = %d", flash16);
    } else {
      ESP_LOGI(TAG, "rssilimit set to default %d", cfg.rssilimit);
      saveConfig();
    }

    nvs_close(my_handle);
    ESP_LOGI(TAG, "Done");

    // put actions to be triggered after config loaded here
    
    #ifdef HAS_ANTENNA_SWITCH // set antenna type, if device has one
      antenna_select(cfg.wifiant);
    #endif
    }
}
