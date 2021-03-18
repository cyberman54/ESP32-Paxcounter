#include "libpax_helpers.h"

// libpax payload
#if LIBPAX
struct count_payload_t count_from_libpax;
uint16_t volatile libpax_macs_ble, libpax_macs_wifi;

void process_count(void) {
  printf("pax: %d; %d; %d;\n", count_from_libpax.pax, count_from_libpax.wifi_count, count_from_libpax.ble_count);
  libpax_macs_ble = count_from_libpax.ble_count;
  libpax_macs_wifi = count_from_libpax.wifi_count;
}

void init_libpax() {
      libpax_counter_init(process_count, &count_from_libpax, 60*1000, 1); 
      libpax_counter_start();
}
#endif