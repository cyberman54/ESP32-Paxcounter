
// Basic Config
#include "globals.h"

#if (VENDORFILTER)
#include "vendor_array.h"
#endif

// Local logging tag
static const char TAG[] = __FILE__;

uint16_t salt;

uint16_t get_salt(void) {
  salt = (uint16_t)random(65536); // get new 16bit random for salting hashes
  return salt;
}

int8_t isBeacon(uint64_t mac) {
  it = std::find(beacons.begin(), beacons.end(), mac);
  if (it != beacons.end())
    return std::distance(beacons.begin(), it);
  else
    return -1;
}

// Display a key
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb) {
  const uint8_t *p;
  char keystring[len + 1] = "", keybyte[3];
  for (uint8_t i = 0; i < len; i++) {
    p = lsb ? key + len - i - 1 : key + i;
    sprintf(keybyte, "%02X", *p);
    strncat(keystring, keybyte, 2);
  }
  ESP_LOGI(TAG, "%s: %s", name, keystring);
}

uint64_t macConvert(uint8_t *paddr) {
  uint64_t *mac;
  mac = (uint64_t *)paddr;
  return (__builtin_bswap64(*mac) >> 8);
}

bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type) {

  if (!salt) // ensure we have salt (appears after radio is turned on)
    return false;

  char buff[10]; // temporary buffer for printf
  bool added = false;
  int8_t beaconID;    // beacon number in test monitor mode
  uint16_t hashedmac; // temporary buffer for generated hash value
  uint32_t *mac;      // temporary buffer for shortened MAC

  // only last 3 MAC Address bytes are used for MAC address anonymization
  // but since it's uint32 we take 4 bytes to avoid 1st value to be 0.
  // this gets MAC in msb (= reverse) order, but doesn't matter for hashing it.
  mac = (uint32_t *)(paddr + 2);

#if (VENDORFILTER)
  uint32_t *oui; // temporary buffer for vendor OUI
  oui = (uint32_t *)paddr;

  // use OUI vendor filter list only on Wifi, not on BLE
  if ((sniff_type == MAC_SNIFF_BLE) ||
      std::find(vendors.begin(), vendors.end(), __builtin_bswap32(*oui) >> 8) !=
          vendors.end()) {
#endif

    // salt and hash MAC, and if new unique one, store identifier in container
    // and increment counter on display
    // https://en.wikipedia.org/wiki/MAC_Address_Anonymization

    snprintf(buff, sizeof(buff), "%08X",
             *mac + (uint32_t)salt);      // convert unsigned 32-bit salted MAC
                                          // to 8 digit hex string
    hashedmac = rokkit(&buff[3], 5);      // hash MAC 8 digit -> 5 digit
    auto newmac = macs.insert(hashedmac); // add hashed MAC, if new unique
    added = newmac.second ? true
                          : false; // true if hashed MAC is unique in container

    // Count only if MAC was not yet seen
    if (added) {
      // increment counter and one blink led
      if (sniff_type == MAC_SNIFF_WIFI) {
        macs_wifi++; // increment Wifi MACs counter
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
        blink_LED(COLOR_GREEN, 50);
#endif
      }
#if (BLECOUNTER)
      else if (sniff_type == MAC_SNIFF_BLE) {
        macs_ble++; // increment BLE Macs counter
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
        blink_LED(COLOR_MAGENTA, 50);
#endif
      }
#endif

      // in beacon monitor mode check if seen MAC is a known beacon
      if (cfg.monitormode) {
        beaconID = isBeacon(macConvert(paddr));
        if (beaconID >= 0) {
          ESP_LOGI(TAG, "Beacon ID#%d detected", beaconID);
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
          blink_LED(COLOR_WHITE, 2000);
#endif
          payload.reset();
          payload.addAlarm(rssi, beaconID);
          SendPayload(BEACONPORT, prio_high);
        }
      };

    } // added

    // Log scan result
    ESP_LOGV(TAG,
             "%s %s RSSI %ddBi -> salted MAC %s -> Hash %04X -> WiFi:%d  "
             "BLTH:%d -> "
             "%d Bytes left",
             added ? "new  " : "known",
             sniff_type == MAC_SNIFF_WIFI ? "WiFi" : "BLTH", rssi, buff,
             hashedmac, macs_wifi, macs_ble, getFreeRAM());

#if (VENDORFILTER)
  } else {
    // Very noisy
    // ESP_LOGD(TAG, "Filtered MAC %02X:%02X:%02X:%02X:%02X:%02X",
    // paddr[0],paddr[1],paddr[2],paddr[3],paddr[5],paddr[5]);
  }
#endif

  // True if MAC WiFi/BLE was new
  return added; // function returns bool if a new and unique Wifi or BLE mac was
                // counted (true) or not (false)
}