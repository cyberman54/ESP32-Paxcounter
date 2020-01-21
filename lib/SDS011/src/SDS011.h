// SDS011 dust sensor PM2.5 and PM10
// ---------------------------------
//
// By R. Zschiegner (rz@madavi.de)
// April 2016
//
// Documentation:
//    - The iNovaFitness SDS011 datasheet
//

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <SoftwareSerial.h>

// Definition SDS011 sensor 'commands'
#define SDS_START_CMD             1
#define SDS_STOP_CMD              2
#define SDS_CONTINUOUS_MODE_CMD   3
#define SDS_VERSION_DATE_CMD      4

class SDS011 {
  public:
    SDS011(void);
    void begin(uint8_t pin_rx, uint8_t pin_tx);
    void begin(HardwareSerial* serial);
    void begin(SoftwareSerial* serial);
    int read(float *p25, float *p10);
    void sleep();
    void wakeup();
    void contmode( int );
  private:
    void SDS_cmd(const uint8_t);
    uint8_t calcChecksum( byte *);
    uint8_t _pin_rx, _pin_tx;
    Stream *sds_data;
};
