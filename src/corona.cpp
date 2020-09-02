// routines for counting the number of devices which advertise Exposure
// Notification Service e.g. "Corona Warn App" in Germany

// copied from https://github.com/kmetz/BLEExposureNotificationBeeper
// (c) by Kaspar Metz
// modified for use in the Paxcounter by AQ

#if (COUNT_ENS)

// Local logging tag
static const char TAG[] = __FILE__;

#define BT_BD_ADDR_HEX(addr)                                                   \
  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#include "corona.h"

// When to forget old senders.
#define FORGET_AFTER_MINUTES 2

// array of timestamps for seen notifiers
static std::map<uint16_t, unsigned long> cwaSeenNotifiers;

// Remove notifiers last seen over FORGET_AFTER_MINUTES ago.
void cwa_clear() {
  /*

    #ifdef SOME_FORM_OF_DEBUG
  ESP_LOGD(TAG, "CWA: forget old notifier: %d", cwaSeenNotifiers.size());
    for (auto const &notifier : cwaSeenNotifiers) {
      ESP_LOGD(TAG, "CWA forget <%X>", notifier.first);
      //    }
    }
  #endif

  */

  // clear everything, otherwise we would count the same device again, as in the
  // next cycle it likely will advertise with a different hash-value
  cwaSeenNotifiers.clear();
}

// return the total number of devices seen advertising ENS
uint16_t cwa_report(void) { return cwaSeenNotifiers.size(); }

bool cwa_init(void) {
  ESP_LOGD(TAG, "init BLE-scanner for ENS");
  return true;
}

// similar to mac_add(), found in macsniff.cpp, for comments look into this function
bool cwa_mac_add(uint16_t hashedmac) {

  bool added = false;
  
  ESP_LOGD(TAG, "hashed ENS mac = %X, ENS count = %d (total=%d)", hashedmac,
           cwaSeenNotifiers.count(hashedmac), cwaSeenNotifiers.size());
  added = !(cwaSeenNotifiers.count(hashedmac) > 0);

  // Count only if this ENS MAC was not yet seen
  if (added) {
    cwaSeenNotifiers[hashedmac] = millis(); // last seen at ....
    ESP_LOGD(TAG, "added device with active ENS");
  }

  // True if ENS MAC was new
  return added;
}

#endif
