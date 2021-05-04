#if (HAS_TRAFFIC_LIGHT_LED)

// Local logging tag
static const char TAG[] = __FILE__;

#include "traffic_light_led.h"
#include "Adafruit_NeoPixel.h"

Adafruit_NeoPixel *pixels;

// init
void traffic_light_led_init() {
  
  pixels = new Adafruit_NeoPixel(3, TRAFFIC_LIGHT_PIN, TRAFFIC_LIGHT_FORMAT);
  pixels->begin();
  pixels->setBrightness(50);
  pixels->clear();
  pixels->setPixelColor(0, 0, 0, 0);
  pixels->setPixelColor(1, 0, 0, 0);
  pixels->setPixelColor(2, 0, 0, 0);
  pixels->show();
  return;
}

// set Pixel color
void set_traffic_light_led(int c1, int c2, int c3) {
	 pixels->clear();
	 pixels->setPixelColor(0, c1, c2, c3);
	 pixels->show();
	}

#endif 
