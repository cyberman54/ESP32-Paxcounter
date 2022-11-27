#include "libpax_helpers.h"


// libpax payload
struct count_payload_t count_from_libpax;

void init_libpax(void) {
  libpax_counter_init(setSendIRQ, &count_from_libpax, cfg.sendcycle * 2,
                      cfg.countermode);
  libpax_counter_start();
}