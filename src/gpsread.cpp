#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

// read GPS data and cast to global struct
void gps_read(){
    gps_status.latitude     =   (uint32_t) (gps.location.lat() * 1000000);
    gps_status.longitude    =   (uint32_t) (gps.location.lng() * 1000000);
    gps_status.satellites   =   (uint8_t) gps.satellites.value();
    gps_status.hdop         =   (uint16_t) gps.hdop.value();
    gps_status.altitude     =   (uint16_t) gps.altitude.meters(); 
}

// GPS serial feed FreeRTos Task
void gps_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check

    #ifdef GPS_SERIAL
        HardwareSerial GPS_Serial(1);  
    #endif

    #ifdef GPS_I2C
        // to be done
    #endif

    while(1) {

        if (cfg.gpsmode)
        {
            #ifdef GPS_SERIAL
                // serial connect to GPS device
                GPS_Serial.begin(GPS_SERIAL);
                
                while(cfg.gpsmode) {
                    // feed GPS decoder with serial NMEA data from GPS device
                    while (GPS_Serial.available()) {
                        gps.encode(GPS_Serial.read());
                        vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog
                    }
                }    
                // after GPS function was disabled, close connect to GPS device
                GPS_Serial.end();
            #endif

            #ifdef GPS_I2C
                // I2C connect to GPS device
                
                /*
                to be done
                */

            #endif
        }
       
        vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog

    } // end of infinite loop

} // gps_loop()
    
#endif // HAS_GPS