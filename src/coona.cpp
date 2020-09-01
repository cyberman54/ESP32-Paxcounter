// routines for counting the number of available Corona Warn Apps (CWA)

// copied from https://github.com/kmetz/BLEExposureNotificationBeeper
// (c) by Kaspar Metz
// modified for use in the Paxcounter by AQ

#if (COUNT_CWA)

// Local logging tag
static const char TAG[] = __FILE__;

#define BT_BD_ADDR_HEX(addr)                                                   \
  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#include "corona.h"

// taken from macsniff.cpp
extern uint16_t salt;    
extern uint16_t hashedmac;

// When to forget old senders.
#define FORGET_AFTER_MINUTES 2

static std::map<uint16_t, unsigned long> cwaSeenNotifiers;


/**
 * Remove notifiers last seen over FORGET_AFTER_MINUTES ago.
 */
void cwa_clear() {
  ESP_LOGD(TAG, "CWA: forget old notifier: %d", cwaSeenNotifiers.size());

//#ifdef SOME_FORM_OF_DEBUG
  for (auto const &notifier : cwaSeenNotifiers) {
      ESP_LOGI(TAG, "CWA forget <%X>", notifier.first);
//    }
  }
//#endif
    /* clear everything,
     * otherwise we will count the same device again
     * as in the next cycle it will get a differetn hash-value
     */
    cwaSeenNotifiers.clear();  
}

/* return the numbers of active CWAs found
 */
uint16_t cwa_report(void) {
  return cwaSeenNotifiers.size();
}

bool cwa_init(void)
{
  ESP_LOGD(TAG, "init of BLE-scanner for CWA");
  return true;

}

// similar to mac_add(), found in macsniff.cpp
// for comments pls. look into this function
bool cwa_mac_add(uint8_t* paddr) {
  
  // are we too early?
  if ( hashedmac <= 0 )
      return false;   // YES -> return

  bool added = false;
  ESP_LOGD(TAG, "Device address (bda): %02x:%02x:%02x:%02x:%02x:%02x",
               BT_BD_ADDR_HEX(paddr));
  
  ESP_LOGD(TAG, "hasehd mac = %X, count = %d (total=%d)", hashedmac, cwaSeenNotifiers.count(hashedmac), cwaSeenNotifiers.size());
    added = !(cwaSeenNotifiers.count(hashedmac) > 0);

    // Count only if MAC was not yet seen
    if (added) {
        ESP_LOGD(TAG, "added device with active CWA");
   }

   cwaSeenNotifiers[hashedmac] = millis(); // last seen at ....

  // True if MAC WiFi/BLE was new
  return added; 
}

#endif 
