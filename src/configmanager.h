/* struct holding devices's runtime configuration */

typedef struct {
  int8_t lorasf;                       // 7-12, lora spreadfactor
  int8_t txpower;                      // 2-15, lora tx power
  int8_t adrmode;                      // 0=disabled, 1=enabled
  int8_t screensaver;                  // 0=disabled, 1=enabled
  int8_t screenon;                     // 0=disabled, 1=enabled
  int8_t countermode;                  // 0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed
  int16_t rssilimit;                   // threshold for rssilimiter, negative value!
  int8_t wifiscancycle;                // wifi scan cycle [seconds/2]
  int8_t wifichancycle;                // wifi channel switch cycle [seconds/100]
  int8_t blescancycle;                 // BLE scan cycle [seconds]
  int8_t blescan;                      // 0=disabled, 1=enabled
  char version[10];                    // Firmware version
  } configData_t;
