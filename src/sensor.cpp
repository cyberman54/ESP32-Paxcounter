/*
As an example of how to include a custom sensor I will use an scd30 C02 sensor
	  https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library
    https://www.sensirion.com/de/umweltsensoren/kohlendioxidsensor/kohlendioxidsensoren-scd30/
		https://www.sparkfun.com/products/15112
  
  sensor is in parallel to the dispaly on i2c bus with address 0x61 
 
  - in sensor.cpp
      - include scd30-master lib from sparkFun
      - in function void sensor_init(void) put one time inti code 
      - in function uint8_t *sensor_read(uint8_t sensor) -> case 2: read our sensor
  - in payload.cpp
      -include sensor.h
  
  - in paxcounter.conf   (define are still in use in some other part of the code)
      - #define HAS_SENSOR_1                    1       
      - #define HAS_SENSOR_2                    1       
      - #define HAS_SENSOR_3                    0
  - 
  
  - change js parser 
     - I use payload encoder: 2=Packed
     if (port === 11) {
        // scd30 sensor data     
        return decode(bytes, [uint16, ufloat, ufloat], ['C02', 'temp', 'humidity']);
    }
	  return decoded;
*/

// Basic Config
#include "globals.h"
#include "sensor.h"

// add include and definition for scd30
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h"
SCD30 airSensor;
bool scd30_error = false;

#if (COUNT_ENS)
#include "payload.h"
#include "corona.h"
#include "macsniff.h"
extern PayloadConvert payload;
#endif



// Local logging tag
static const char TAG[] = __FILE__;

#define SENSORBUFFER                                                           \
  10 // max. size of user sensor data buffer in bytes [default=20]

void sensor_init(void) {

  // this function is called during device startup
  // put your user sensor initialization routines here

// init code for scd30
 Wire.begin();
   
  if (airSensor.begin() == false)
  {
    ESP_LOGE(TAG, "Check wiring von SCD30 Sensor");
    scd30_error = true;
  }
  // we do not need to do a measurment ever 2 second (default), because we do not
  // send every 2 second
  airSensor.setMeasurementInterval(60);
  // my location is 9m over sealevel
  airSensor.setAltitudeCompensation(9);

/* here you can add additional init code like
- bool setMeasurementInterval(uint16_t interval);
-	bool setAmbientPressure(uint16_t pressure_mbar);
- bool setAltitudeCompensation(uint16_t altitude);
-	bool setAutoSelfCalibration(bool enable);
-	bool setForcedRecalibrationFactor(uint16_t concentration);
-	bool setTemperatureOffset(float tempOffset);
*/
}

uint8_t sensor_mask(uint8_t sensor_no) {
  switch (sensor_no) {
  case 0:
    return (uint8_t)COUNT_DATA;
  case 1:
    return (uint8_t)SENSOR1_DATA;
  case 2:
    return (uint8_t)SENSOR2_DATA;
  case 3:
    return (uint8_t)SENSOR3_DATA;
  case 4:
    return (uint8_t)BATT_DATA;
  case 5:
    return (uint8_t)GPS_DATA;
  case 6:
    return (uint8_t)MEMS_DATA;
  case 7:
    return (uint8_t)ALARM_DATA;
  default:
    return 0;
  }
}

uint8_t *sensor_read(uint8_t sensor) {

  static uint8_t buf[SENSORBUFFER] = {0};
  uint8_t length = 3;

  switch (sensor) {

  case 1:

    // insert user specific sensor data frames here
    // note: Sensor1 fields are used for ENS count, if ENS detection enabled
#if (COUNT_ENS)
    // some correction for ENS (Port10) 
      buf[0] = 0; // if ENS, we need 0 bytes, the addCount will add 2 bytes
    // if (cfg.enscount)
      payload.addCount(cwa_report(), MAC_SNIFF_BLE_ENS);
      
#else
    buf[0] = length;
    buf[1] = 0x01;
    buf[2] = 0x02;
    buf[3] = 0x03;
#endif
    break;
  case 2:

    // code for scd30   HJB 18032021
    ESP_LOGD(TAG, "read data from sensor 2");
    if (!scd30_error) {
     if (airSensor.dataAvailable())
      {
        uint16_t co2=0; 
        co2  = airSensor.getCO2();
        float temp =0;
        temp = airSensor.getTemperature();
        float humi=0; 
        humi = airSensor.getHumidity();
        ESP_LOGD(TAG, "Co2 %d", co2);
        ESP_LOGD(TAG, "Temp %f", temp);
        ESP_LOGD(TAG, "Humi %f", humi);
    
        temp = temp * 100;  // to transport this as byte via Lora 
        humi = humi * 100;  // to transport this as byte via Lora
        buf[0] = 6;         // lengh is 6 byte data
        buf[1] = lowByte(co2);
        buf[2] = highByte(co2);
        buf[3] = lowByte((uint16_t)temp);
        buf[4] = highByte((uint16_t)temp);
        buf[5] = lowByte((uint16_t)humi);
        buf[6] = highByte((uint16_t)humi);
      }
    }
    break;

  case 3:

    buf[0] = length;
    buf[1] = 0x01;
    buf[2] = 0x02;
    buf[3] = 0x03;
    break;
  }

  return buf;
}
