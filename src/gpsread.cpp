#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

// GPS Read FreeRTos Task
void gps_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check

    HardwareSerial GPS_Serial(1);                 
    //GPS_Serial.begin(9600, SERIAL_8N1, 12, 15);
    GPS_Serial.begin(HAS_GPS);

    while(1) {

        while (GPS_Serial.available()) {
            my_gps.encode(GPS_Serial.read());
        }

        vTaskDelay(1/portTICK_PERIOD_MS); // reset watchdog
    }    
}

#endif // HAS_GPS