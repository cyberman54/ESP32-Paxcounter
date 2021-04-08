// routines for fetching data from the SCD30-sensor

#if (HAS_SCD30)
#include "scd30read.h"

#if (HAS_TRAFFIC_LIGHT_LED)
		#include "traffic_light_led.h"
	#endif

// Local logging tag
static const char TAG[] = __FILE__;
static SCD30 scd30Sensor;
// the results of the sensor:
static float temp, humi;
static uint16_t co2;
boolean scd30_error = false;

// init
bool scd30_init() {
	
Wire.begin();
if (scd30Sensor.begin() == false)
  {
    ESP_LOGE(TAG, "Check wiring von SCD30 Sensor");
    scd30_error = true;
	  return false;
    }
  // we do not need to do a measurment ever 2 second (default), because we do not
  // send every 2 second
  
  scd30Sensor.setMeasurementInterval(60);
  // my location is 9m over sealevel
  scd30Sensor.setAltitudeCompensation(9);


  temp = humi = 0.0;
  co2 = 0;
  scd30_error = false;

#if (HAS_TRAFFIC_LIGHT_LED)
   traffic_light_led_init();
  #endif

  
  return true;
}

// reading data:
void scd30_read() {
	ESP_LOGD(TAG, "read data from scd30_sensor");
    if (!scd30_error) {
     if (scd30Sensor.dataAvailable())
      {
        co2  = scd30Sensor.getCO2();
        temp = scd30Sensor.getTemperature();
        humi = scd30Sensor.getHumidity();
        ESP_LOGD(TAG, "Co2 %d", co2);
        ESP_LOGD(TAG, "Temp %f", temp);
        ESP_LOGD(TAG, "Humi %f", humi);

  #if (HAS_TRAFFIC_LIGHT_LED)
ESP_LOGD(TAG, "set Traffic Light with co2 value");
if (co2 < 1000 ) 
	{
	set_traffic_light_led(255,0,0);
	}
	else if (co2 >= 1000 && co2 < 1500) 
	{
    set_traffic_light_led(255,255,0);
	}
	else 
	{
  set_traffic_light_led(0,255,0);
	};
#endif

	  }	
	  else ESP_LOGD(TAG, "no Data Avialable");
	}
	else   ESP_LOGE(TAG, "scd30 sensor has error");



  return;
}

void scd30_store(scd30Status_t *scd30_store) {
  scd30_store->temp = temp;
  scd30_store->humi = humi;
  scd30_store->co2 = co2;
}
#endif

// test
