
// Basic Config
#include "globals.h"
#include "macsniff.h"

#if (VENDORFILTER)
#include "vendor_array.h"
#endif

// Local logging tag
static const char TAG[] = __FILE__;

QueueHandle_t MacQueue;
TaskHandle_t macProcessTask;

uint16_t salt = 0;

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
    snprintf(keybyte, 3, "%02X", *p);
    strncat(keystring, keybyte, 2);
  }
  ESP_LOGI(TAG, "%s: %s", name, keystring);
}

uint64_t macConvert(uint8_t *paddr) {
  uint64_t *mac;
  mac = (uint64_t *)paddr;
  return (__builtin_bswap64(*mac) >> 16);
}

esp_err_t macQueueInit() {
  _ASSERT(MAC_QUEUE_SIZE > 0);
  MacQueue = xQueueCreate(MAC_QUEUE_SIZE, sizeof(MacBuffer_t));
  if (MacQueue == 0) {
    ESP_LOGE(TAG, "Could not create MAC processing queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "MAC processing queue created, size %d Bytes",
           MAC_QUEUE_SIZE * sizeof(MacBuffer_t));

  xTaskCreatePinnedToCore(mac_process,     // task function
                          "mac_process",   // name of task
                          2048,            // stack size of task
                          (void *)1,       // parameter of the task
                          1,               // priority of the task
                          &macProcessTask, // task handle
                          1);              // CPU core

  return ESP_OK;
}

// sniffed MAC processing task
void mac_process(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  MacBuffer_t MacBuffer;

  while (1) {

    // fetch next or wait for incoming MAC from sniffing queue
    if (xQueueReceive(MacQueue, &MacBuffer, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // update traffic indicator
    rf_load = uxQueueMessagesWaiting(MacQueue);
    // process fetched mac
    mac_analyze(MacBuffer);
  }
  delay(2); // yield to CPU
}

// enqueue message in MAC processing queue
void IRAM_ATTR mac_add(uint8_t *paddr, int8_t rssi, snifftype_t sniff_type) {

  MacBuffer_t MacBuffer;

  MacBuffer.rssi = rssi;
  MacBuffer.sniff_type = sniff_type;
  memcpy(MacBuffer.mac, paddr, 6);

  if (xQueueSendToBackFromISR(MacQueue, (void *)&MacBuffer, (TickType_t)0) !=
      pdPASS)
    ESP_LOGW(TAG, "Dense radio traffic, packet lost!");
}

uint16_t mac_analyze(MacBuffer_t MacBuffer) {

  if (salt == 0) // ensure we have salt (appears after radio is turned on)
    return 0;

  if ((cfg.rssilimit) &&
      (MacBuffer.rssi < cfg.rssilimit)) { // rssi is negative value
    ESP_LOGI(TAG, "%s RSSI %d -> ignoring (limit: %d)",
             (MacBuffer.sniff_type == MAC_SNIFF_WIFI) ? "WIFI" : "BLTH",
             MacBuffer.rssi, cfg.rssilimit);
    return 0;
  }

  // in beacon monitor mode check if seen MAC is a known beacon
  if (cfg.monitormode) {
    int8_t beaconID = isBeacon(macConvert(MacBuffer.mac));
    if (beaconID >= 0) {
      ESP_LOGI(TAG, "Beacon ID#%d detected", beaconID);
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
      blink_LED(COLOR_WHITE, 2000);
#endif
      payload.reset();
      payload.addAlarm(MacBuffer.rssi, beaconID);
      SendPayload(BEACONPORT, prio_high);
    }
  };

#if (VENDORFILTER)
  uint32_t *oui; // temporary buffer for vendor OUI
  oui = (uint32_t *)MacBuffer.mac;
  // if we find OUI on vendor filter list we don't analyze and return early
  if (std::find(vendors.begin(), vendors.end(), __builtin_bswap32(*oui) >> 8) ==
      vendors.end())
    return 0;
#endif

  char buff[10]; // temporary buffer for printf
  uint32_t *mac; // temporary buffer for shortened MAC

  // only last 3 MAC Address bytes are used for MAC address anonymization
  // but since it's uint32 we take 4 bytes to avoid 1st value to be 0.
  // this gets MAC in msb (= reverse) order, but doesn't matter for hashing it.
  mac = (uint32_t *)(MacBuffer.mac + 2);

  // salt and hash MAC, and if new unique one, store identifier in container
  // and increment counter on display
  // https://en.wikipedia.org/wiki/MAC_Address_Anonymization

  snprintf(buff, sizeof(buff), "%08X",
           *mac + (uint32_t)salt); // convert unsigned 32-bit salted MAC
                                   // to 8 digit hex string
  uint16_t hashedmac = rokkit(&buff[3], 5); // hash MAC 8 digit -> 5 digit
  auto newmac = macs.insert(hashedmac);     // add hashed MAC, if new unique
  bool added =
      newmac.second ? true : false; // true if hashed MAC is unique in container

  // Count only if MAC was not yet seen
  if (added) {

    switch (MacBuffer.sniff_type) {

    case MAC_SNIFF_WIFI:
      macs_wifi++; // increment Wifi MACs counter
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
      blink_LED(COLOR_GREEN, 50);
#endif
      break;

#if (BLECOUNTER)
    case MAC_SNIFF_BLE:
      macs_ble++; // increment BLE Macs counter
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
      blink_LED(COLOR_MAGENTA, 50);
#endif
      break;

#if (COUNT_ENS)
    case MAC_SNIFF_BLE_ENS:
      macs_ble++;             // increment BLE Macs counter
      cwa_mac_add(hashedmac); // process ENS beacon
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
      blink_LED(COLOR_WHITE, 50);
#endif
      break;

#endif // COUNT_ENS
#endif // BLECOUNTER

    } // switch
  }   // added

  // Log scan result
  ESP_LOGV(TAG,
           "%s %s RSSI %ddBi -> salted MAC %s -> Hash %04X -> WiFi:%d  "
           "BLTH:%d "
#if (COUNT_ENS)
           "(CWA:%d)"
#endif
           "-> %d Bytes left",
           added ? "new  " : "known",
           MacBuffer.sniff_type == MAC_SNIFF_WIFI ? "WiFi" : "BLTH",
           MacBuffer.rssi, buff, hashedmac, macs_wifi, macs_ble,
#if (COUNT_ENS)
           cwa_report(),
#endif
           getFreeRAM());

  // if an unknown Wifi or BLE mac was counted, return hash of this mac, else 0
  return (added ? hashedmac : 0);
}
