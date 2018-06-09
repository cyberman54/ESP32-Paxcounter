#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

// read GPS data and cast to global struct
void gps_read(){
    gps_status.latitude     =   (float) gps.location.lat();
    gps_status.longitude    =   (float) gps.location.lng();
    gps_status.satellites   =   (uint8_t) gps.satellites.value();
    gps_status.hdop         =   (uint16_t) gps.hdop.value();
    gps_status.altitude     =   (uint16_t) gps.altitude.meters(); 
}

// GPS serial feed FreeRTos Task
void gps_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check

    HardwareSerial GPS_Serial(1);  

    while(1) {

        if (cfg.gpsmode)
        {
            // if GPS function is enabled try serial connect to GPS device
            GPS_Serial.begin(HAS_GPS);
            
            while(cfg.gpsmode) {
                // feed GPS decoder with serial NMEA data from GPS device
                while (GPS_Serial.available()) {
                    gps.encode(GPS_Serial.read());
                    vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog
                }
            }    
            // after GPS function was disabled, close connect to GPS device
            GPS_Serial.end();
        }
       
        vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog

    } // end of infinite loop

} // gps_loop()
    
#endif // HAS_GPS