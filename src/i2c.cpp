// Basic config
#include "globals.h"
#include "i2c.h"


SemaphoreHandle_t I2Caccess;

void i2c_init(void) {
  Wire.setPins(MY_DISPLAY_SDA, MY_DISPLAY_SCL);
  Wire.begin();
}

void i2c_deinit(void) { Wire.end(); }

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

  ESP_LOGI(TAG, "Starting I2C bus scan...");

  memset(&bbi2c, 0, sizeof(bbi2c));
  bbi2c.bWire = 1;
  bbi2c.iSDA = MY_DISPLAY_SDA;
  bbi2c.iSCL = MY_DISPLAY_SCL;
  I2CInit(&bbi2c, 100000L); // Scan at 100KHz low speed
  delay(100);               // allow devices to power up

  uint8_t map[16];
  uint8_t i;
  int iDevice, iCount;
  uint32_t iDevCapab;
  char szName[15];

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
        iDevice = I2CDiscoverDevice(&bbi2c, i, &iDevCapab);
        I2CGetDeviceName(iDevice, szName);
        ESP_LOGI(TAG, "Device found at 0x%X, type = %s", i,
                 szName); // show the device name as a string
      }
    } // for i
    ESP_LOGI(TAG, "%u I2C device(s) found", iCount);
  }
}

// functions for i2c r/w access, mutexing is done by Wire.cpp
int i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
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
  return ret ? ret : 0xFF;
}

int i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  uint8_t ret = 0;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  for (uint16_t i = 0; i < len; i++) {
    Wire.write(data[i]);
  }
  ret = Wire.endTransmission();

  return ret ? ret : 0xFF;
}