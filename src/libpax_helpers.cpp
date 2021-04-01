#include "libpax_helpers.h"

// Local logging tag
static const char TAG[] = __FILE__;

// libpax payload
struct count_payload_t count_from_libpax;
uint16_t volatile libpax_macs_ble, libpax_macs_wifi;

void process_count(void) {
  ESP_LOGD(TAG, "pax: %d / %d / %d", count_from_libpax.pax,
           count_from_libpax.wifi_count, count_from_libpax.ble_count);
  libpax_macs_ble = count_from_libpax.ble_count;
  libpax_macs_wifi = count_from_libpax.wifi_count;
  setSendIRQ();
}

void init_libpax(void) {
  libpax_counter_init(process_count, &count_from_libpax, cfg.sendcycle * 2 * 1000,
                      1);
  libpax_counter_start();
}