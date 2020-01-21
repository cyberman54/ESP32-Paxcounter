// routines for fetching data from the SDS011-sensor

// Local logging tag
static const char TAG[] = __FILE__;

#include <sds011read.h>

// UART(2) is unused in this project
static HardwareSerial sdsSerial(2);    // so we use it here
static SDS011 sdsSensor;               // fine dust sensor

// the results of the sensor:
float pm25;              
float pm10;

// init
bool sds011_init()
{
    pm25 = pm10 = 0.0;
    sdsSerial.begin(9600, SERIAL_8N1, ESP_PIN_RX, ESP_PIN_TX);
    sdsSensor.begin (&sdsSerial);
    sdsSensor.contmode(0);              // for safety: wakeup/sleep - if we want it we do it by ourselves
    sdsSensor.wakeup();                 // always wake up
    return true;
}
// reading data:
void sds011_loop()
{
    pm25 = pm10 = 0.0;
    int sdsErrorCode = sdsSensor.read(&pm25, &pm10);
    if (!sdsErrorCode) 
    {
        ESP_LOGD(TAG, "SDS011 error: %d", sdsErrorCode);
    } 
    return;
}
