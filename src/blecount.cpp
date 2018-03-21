// not using this seems to break U8X8LIB
//#include <Arduino.h>

// Basic Config
#include "main.h"
#include "globals.h"
//#include "loraconf.h"

#ifdef BLECOUNTER
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Local logging tag
static const char *TAG = "blecount";

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
    }
};

void BLECount() {
    u8x8.clearLine(3);
    u8x8.drawString(0,3,"BLE Scan...");
    BLEDevice::init("PaxCnt");
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(cfg.blescancycle);
    u8x8.clearLine(3);
    u8x8.setCursor(0,3);
    blenum=foundDevices.getCount();
    u8x8.printf("BLE#: %4i",blenum);
}
#endif