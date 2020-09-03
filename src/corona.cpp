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

// When to forget old senders ** currently not used **
#define FORGET_AFTER_MINUTES 2

// array of timestamps for seen notifiers: hash -> timestamp[ms]
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

void cwa_mac_add(uint16_t hashedmac) {
  cwaSeenNotifiers[hashedmac] = millis(); // hash last seen at ....
}

#endif
