// Basic config
#include "globals.h"
#include "i2c.h"

// Local logging tag
static const char TAG[] = __FILE__;

void i2c_init(void) { Wire.begin(MY_DISPLAY_SDA, MY_DISPLAY_SCL, 100000); }

void i2c_deinit(void) {
  Wire.~TwoWire(); // shutdown/power off I2C hardware
  // configure pins as input to save power, because Wire.end() enables pullups
  pinMode(MY_DISPLAY_SDA, INPUT);
  pinMode(MY_DISPLAY_SCL, INPUT);
}

void i2c_scan(void) {

  // parts of the code in this function were taken from:
  //
  // Copyright (c) 2019 BitBank Software, Inc.
  // Written by Larry Bank
  // email: bitbank@pobox.com
  // Project started 25/02/2019
  //
  // This program is free software: you can redistribute it and/or modify
  // it under the terms of the GNU General Public License as published by
  // the Free Software Foundation, either version 3 of the License, or
  // (at your option) any later version.
  //
  // This program is distributed in the hope that it will be useful,
  // but WITHOUT ANY WARRANTY; without even the implied warranty of
  // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  // GNU General Public License for more details.
  //
  // You should have received a copy of the GNU General Public License
  // along with this program.  If not, see <http://www.gnu.org/licenses/>.

  BBI2C bbi2c;

  const char *szNames[]  = {"Unknown","SSD1306","SH1106","VL53L0X","BMP180", "BMP280","BME280",
                "MPU-60x0", "MPU-9250", "MCP9808","LSM6DS3", "ADXL345", "ADS1115","MAX44009",
                "MAG3110", "CCS811", "HTS221", "LPS25H", "LSM9DS1","LM8330", "DS3231", "LIS3DH",
                "LIS3DSH","INA219","SHT3X","HDC1080","MPU6886","BME680", "AXP202", "AXP192", "24AA02XEXX", "DS1307"};

  ESP_LOGI(TAG, "Starting I2C bus scan...");

  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {

    memset(&bbi2c, 0, sizeof(bbi2c));
    bbi2c.bWire = 1; // use wire library, no bitbanging
    bbi2c.iSDA = MY_DISPLAY_SDA;
    bbi2c.iSCL = MY_DISPLAY_SCL;
    I2CInit(&bbi2c, 100000L); // Scan at 100KHz low speed
    delay(100);               // allow devices to power up

    uint8_t map[16];
    uint8_t i;
    int iDevice, iCount;

    I2CScan(&bbi2c, map); // get bitmap of connected I2C devices
    if (map[0] == 0xfe)   // something is wrong with the I2C bus
    {
      ESP_LOGI(TAG, "I2C pins are not correct or the bus is being pulled low "
                    "by a bad device; unable to run scan");
    } else {
      iCount = 0;
      for (i = 1; i < 128; i++) // skip address 0 (general call address) since
                                // more than 1 device can respond
      {
        if (map[i >> 3] & (1 << (i & 7))) // device found
        {
          iCount++;
          iDevice = I2CDiscoverDevice(&bbi2c, i);
          ESP_LOGI(TAG, "Device found at 0x%X, type = %s", i,
                   szNames[iDevice]); // show the device name as a string
        }
      } // for i
      ESP_LOGI(TAG, "%u I2C device(s) found", iCount);
    }

    I2C_MUTEX_UNLOCK(); // release i2c bus access
  } else
    ESP_LOGE(TAG, "I2C bus busy - scan error");
}

// mutexed functions for i2c r/w access
uint8_t i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {

    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    uint8_t cnt = Wire.requestFrom(addr, (uint8_t)len, (uint8_t)1);
    if (!cnt)
      ret = 0xFF;
    uint16_t index = 0;
    while (Wire.available()) {
      if (index > len) {
        ret = 0xFF;
        goto finish;
      }
      data[index++] = Wire.read();
    }

  finish:
    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}

uint8_t i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {

    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint16_t i = 0; i < len; i++) {
      Wire.write(data[i]);
    }
    ret = Wire.endTransmission();

    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret ? ret : 0xFF;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}
