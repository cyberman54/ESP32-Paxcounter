#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";
/*
// GPS read data to global struct
void gps_read(){
    gps_status.latitude = gps.location.lat();
    gps_status.longitude = gps.location.lng();
    gps_status.satellites = (uint8_t) gps.satellites.value();
    gps_status.hdop = (uint8_t) (gps.hdop.value() / 100);
    gps_status.altitude = (uint16_t) gps.altitude.meters(); 
}

/// GPS serial feed FreeRTos Task
void gps_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check

    HardwareSerial GPS_Serial(1);                 
    //GPS_Serial.begin(HAS_GPS);
    GPS_Serial.begin(9600, SERIAL_8N1, 12, 15 );

    while(1) {

        while (GPS_Serial.available()) {
            gps.encode(GPS_Serial.read());
        }

        vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog
    }    
}
*/
#endif // HAS_GPS